#include "protocoldecoder.h"

#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/datatype.h"
#include "flagdata/dataverify.h"

#include <qdatetime.h>
#include <qmap.h>
#include <algorithm>
#include <climits>

PROTOCOL_CODEC_NAMESPACE_BEGIN
    ProtocolDecoder::ProtocolDecoder(QObject* parent)
    : ProtocolCodecInterface(parent)
    , flagCount(0)
    , mBufferMaxSize(10 * 1024 * 1024) //10M
    , mDecodeTimeout(-1)
    , lastDecodeTimestamp(0)
{
}

void ProtocolDecoder::setFlags(ProtocolFlagData* flags) {
    clearCache();
    ProtocolCodecInterface::setFlags(flags);

    static QMap<ProtocolFlag, int> flagWeight = {
            { ProtocolFlag::Flag_Header, 100 },
            { ProtocolFlag::Flag_Address, 100 },
            { ProtocolFlag::Flag_Type, 100 },
            { ProtocolFlag::Flag_Size, 100 },
            { ProtocolFlag::Flag_End, 99 },
            { ProtocolFlag::Flag_Verify, 98 },
            { ProtocolFlag::Flag_Content, 97 },
    };

    QVector<ProtocolFlagData*> flagList;
    for (auto* node = flags; node != nullptr; node = node->next) {
        flagList.append(node);
    }

    // 按权重排序（高权重在前）
    std::stable_sort(flagList.begin(), flagList.end(), [&](ProtocolFlagData* a, ProtocolFlagData* b) {
         int wa = flagWeight.value(a->flag, 0);
         int wb = flagWeight.value(b->flag, 0);
         return wa > wb;
     });

    // 填充 decodeFlags 数组
    flagCount = 0;
    for (auto* f : flagList) {
        if (flagCount >= 256) break;
        decodeFlags[flagCount++] = f;
    }
}

void ProtocolDecoder::addType(int type, const std::function<void(const QByteArray &)> &decoder) {
    typeDecoders.insert(type, decoder);
}

bool ProtocolDecoder::checkRegistered(int type) const {
    return typeDecoders.contains(type);
}

void ProtocolDecoder::addBuffer(const QByteArray &buffer) {
    if (protocolFlags == nullptr || flagCount == 0 || buffer.isEmpty()) {
        return;
    }
    if (mDecodeTimeout > 0) {
        const auto cur = QDateTime::currentMSecsSinceEpoch();
        if (lastDecodeTimestamp != 0 && cur - lastDecodeTimestamp > mDecodeTimeout
            && !bufferCache.isEmpty()) {
            logDecodeError(DecodeError::Timeout,
                QString("receive gap exceeded %1 ms; discarded cached bytes").arg(mDecodeTimeout),
                bufferCache, false);
            clearCache();
        }
        lastDecodeTimestamp = cur;
    }
    bufferCache.append(buffer);

    const int dataSize = bufferCache.size();
    const QByteArray header = protocolFlags->flag == ProtocolFlag::Flag_Header
        ? protocolFlags->getBytesContent() : QByteArray();
    int offset = 0;
    int pendingOffset = -1;
    int noiseStart = 0;
    int diagnosedEnd = 0;
    const auto logNoise = [&](int end) {
        const int start = qMax(noiseStart, diagnosedEnd);
        if (end > start && errorLoggingEnabled(DecodeError::HeaderMismatch)) {
            logDecodeError(DecodeError::HeaderMismatch, "discarded bytes before a matching header",
                           bufferCache.mid(start, end - start), false);
        }
    };

    while (offset < dataSize) {
        if (!header.isEmpty()) {
            const int available = qMin(header.size(), dataSize - offset);
            if (memcmp(bufferCache.constData() + offset, header.constData(), available) != 0) {
                ++offset;
                continue;
            }
            if (available < header.size()) {
                logNoise(offset);
                if (pendingOffset < 0) pendingOffset = offset;
                noiseStart = dataSize;
                break;
            }
        }
        logNoise(offset);
        ProtocolMetaData metadata;
        int frameSize = -1;
        int type = INT_MAX;
        QByteArray content;
        DecodeError error = DecodeError::None;
        QString detail;
        const auto result = decodeFrame(offset, metadataEnable ? &metadata : nullptr,
                                        frameSize, type, content, error, detail);
        if (metadataEnable && metadata.decodeType != INT_MAX) {
            metaData[metadata.decodeType] = metadata;
            lastMetaData = metadata;
        }
        if (result == DecodeResult::Incomplete) {
            if (pendingOffset < 0) pendingOffset = offset;
            diagnosedEnd = dataSize;
            noiseStart = ++offset;
            continue;
        }
        if (result == DecodeResult::Invalid) {
            if (pendingOffset < 0 && errorLoggingEnabled(error)) {
                const bool completeFrame = frameSize > 0 && frameSize <= dataSize - offset;
                int end = completeFrame ? offset + frameSize : dataSize;
                if (!completeFrame && !header.isEmpty()) {
                    const int nextHeader = bufferCache.indexOf(header, offset + header.size());
                    if (nextHeader >= 0) end = nextHeader;
                }
                logDecodeError(error, detail, bufferCache.mid(offset, end - offset), completeFrame);
                diagnosedEnd = qMax(diagnosedEnd, end);
            }
            noiseStart = ++offset;
            continue;
        }

        const auto decoder = typeDecoders.value(type);
        if (!decoder) {
            logDecodeError(DecodeError::UnknownType,
                QString("no decoder registered for type=0x%1").arg(quint32(type), 0, 16),
                bufferCache.mid(offset, frameSize), true);
        } else {
            decoder(content);
        }
        offset += frameSize;
        pendingOffset = -1;
        noiseStart = offset;
    }
    logNoise(dataSize);
    bufferCache = pendingOffset < 0 ? QByteArray() : bufferCache.mid(pendingOffset);
    if (mBufferMaxSize > 0 && bufferCache.size() > mBufferMaxSize) {
        const int discarded = bufferCache.size() - mBufferMaxSize;
        logDecodeError(DecodeError::BufferOverflow,
            QString("cache exceeded limit=%1; discarded oldest bytes").arg(mBufferMaxSize),
            bufferCache.left(discarded), false);
        bufferCache.remove(0, discarded);
    }
}

ProtocolDecoder::DecodeResult ProtocolDecoder::decodeFrame(int offset, ProtocolMetaData* metadata,
    int& frameSize, int& type, QByteArray& content, DecodeError& error, QString& detail) {
    const int available = bufferCache.size() - offset;
    bool addressMismatch = false;
    QString addressDetail;
    const auto byteSize = [](ProtocolFlagData* flag) {
        return flag->flag == ProtocolFlag::Flag_Content
            ? static_cast<ProtocolFlagDataContent*>(flag)->getByteSize() : flag->byteSize;
    };
    const auto measureFrame = [&]() {
        qint64 size = 0;
        for (auto* node = protocolFlags; node != nullptr; node = node->next) {
            const int bytes = byteSize(node);
            if (bytes < 0 || size + bytes > INT_MAX) {
                error = DecodeError::InvalidLength;
                detail = "invalid or unknown content length";
                if (node->flag == ProtocolFlag::Flag_Content) {
                    auto* content = static_cast<ProtocolFlagDataContent*>(node);
                    if (!content->isFixByteSize && !content->isSizeTarget
                        && !content->contentSizeFromType.contains(type)) {
                        error = DecodeError::UnknownType;
                        detail = QString("no content length registered for type=0x%1")
                            .arg(quint32(type), 0, 16);
                    }
                }
                return false;
            }
            size += bytes;
        }
        if (size <= 0) {
            error = DecodeError::InvalidLength;
            detail = "frame length must be positive";
            return false;
        }
        frameSize = int(size);
        return true;
    };

    for (int i = 0; i < flagCount; ++i) {
        auto* flag = decodeFlags[i];
        if (frameSize < 0 && (flag->flag == ProtocolFlag::Flag_End
            || flag->flag == ProtocolFlag::Flag_Verify || flag->flag == ProtocolFlag::Flag_Content)) {
            if (!measureFrame()) return DecodeResult::Invalid;
            if (frameSize > available) return DecodeResult::Incomplete;
            if (addressMismatch) {
                error = DecodeError::AddressMismatch;
                detail = addressDetail;
                return DecodeResult::Invalid;
            }
        }
        if (flag->verify(bufferCache.data(), offset, bufferCache.size(), metadata, nullptr)) {
            if (flag->flag == ProtocolFlag::Flag_Type) {
                type = static_cast<ProtocolFlagDataType*>(flag)->value;
            } else if (flag->flag == ProtocolFlag::Flag_Content) {
                content = static_cast<ProtocolFlagDataContent*>(flag)->contentData;
            }
            continue;
        }
        if (frameSize < 0) {
            qint64 fieldEnd = flag->byteSize;
            for (auto* node = flag->prev; node != nullptr; node = node->prev) fieldEnd += byteSize(node);
            if (fieldEnd > available) return DecodeResult::Incomplete;
        }
        switch (flag->flag) {
            case ProtocolFlag::Flag_Address:
                addressMismatch = true;
                if (errorLoggingEnabled(DecodeError::AddressMismatch)) {
                    int fieldOffset = 0;
                    for (auto* node = flag->prev; node != nullptr; node = node->prev) fieldOffset += byteSize(node);
                    addressDetail = QString("expected=%1 received=%2")
                        .arg(QString::fromLatin1(flag->getBytesContent().toHex(' ')),
                             QString::fromLatin1(bufferCache.mid(offset + fieldOffset, flag->byteSize).toHex(' ')));
                }
                continue;
            case ProtocolFlag::Flag_Size: {
                error = DecodeError::InvalidLength;
                auto* size = static_cast<ProtocolFlagDataSize*>(flag);
                if (errorLoggingEnabled(error)) {
                    detail = QString("declared=%1 maximum=%2").arg(size->dataSize).arg(size->sizeValueMax);
                }
                break;
            }
            case ProtocolFlag::Flag_End:
                error = DecodeError::TailMismatch;
                if (errorLoggingEnabled(error)) {
                    int fieldOffset = 0;
                    for (auto* node = flag->prev; node != nullptr; node = node->prev) fieldOffset += byteSize(node);
                    detail = QString("expected=%1 received=%2")
                        .arg(QString::fromLatin1(flag->getBytesContent().toHex(' ')),
                             QString::fromLatin1(bufferCache.mid(offset + fieldOffset, flag->byteSize).toHex(' ')));
                }
                break;
            case ProtocolFlag::Flag_Verify:
                error = DecodeError::ChecksumMismatch;
                if (errorLoggingEnabled(error)) {
                    auto* verify = static_cast<ProtocolFlagDataVerify*>(flag);
                    int fieldOffset = 0;
                    for (auto* node = flag->prev; node != nullptr; node = node->prev) fieldOffset += byteSize(node);
                    detail = QString("algorithm=%1 calculated=%2 received=%3").arg(flag->dataToString(),
                        QString::fromLatin1(QByteArray(verify->decodeVerifyBuffer, flag->byteSize).toHex(' ')),
                        QString::fromLatin1(bufferCache.mid(offset + fieldOffset, flag->byteSize).toHex(' ')));
                }
                break;
            case ProtocolFlag::Flag_Header:
                error = DecodeError::HeaderMismatch;
                detail = "header mismatch";
                break;
            default:
                error = DecodeError::InvalidLength;
                detail = "invalid field data";
                break;
        }
        return DecodeResult::Invalid;
    }
    if (frameSize < 0 && !measureFrame()) return DecodeResult::Invalid;
    if (addressMismatch) {
        error = DecodeError::AddressMismatch;
        detail = addressDetail;
        return DecodeResult::Invalid;
    }
    return DecodeResult::Complete;
}

bool ProtocolDecoder::errorLoggingEnabled(DecodeError error) const {
    return errorLogFlags.testFlag(error)
        && (debugPtr != nullptr ? debugPtr().isWarningEnabled() : error == DecodeError::UnknownType);
}

void ProtocolDecoder::logDecodeError(DecodeError error, const QString& detail,
    const QByteArray& bytes, bool completeFrame) const {
    if (!errorLogFlags.testFlag(error)) return;
    if (debugPtr == nullptr && error != DecodeError::UnknownType) return;
    const char* name = "unknown";
    switch (error) {
        case DecodeError::HeaderMismatch: name = "header-mismatch"; break;
        case DecodeError::AddressMismatch: name = "address-mismatch"; break;
        case DecodeError::InvalidLength: name = "invalid-length"; break;
        case DecodeError::TailMismatch: name = "tail-mismatch"; break;
        case DecodeError::ChecksumMismatch: name = "checksum-mismatch"; break;
        case DecodeError::UnknownType: name = "unknown-type"; break;
        case DecodeError::Timeout: name = "timeout"; break;
        case DecodeError::BufferOverflow: name = "buffer-overflow"; break;
        default: break;
    }
    const auto message = [&]() {
        return QString("protocol decode error=%1 %2 boundary=%3 bytes=%4 hex=%5")
            .arg(QLatin1String(name), detail,
                 completeFrame ? QStringLiteral("complete-frame") : QStringLiteral("unknown"))
            .arg(bytes.size()).arg(QString::fromLatin1(bytes.toHex(' ')));
    };
    if (debugPtr != nullptr) {
        printWarning(message);
    } else {
        qWarning().noquote() << message();
    }
}

void ProtocolDecoder::setBufferMaxSize(int size) {
    mBufferMaxSize = size;
}

void ProtocolDecoder::setSizeMaxValue(int value) const {
    auto nextPtr = protocolFlags;
    while (nextPtr != nullptr && nextPtr->next != nullptr) {
        if (nextPtr->flag == ProtocolFlag::Flag_Size) {
            dynamic_cast<ProtocolFlagDataSize*>(nextPtr)->setSizeMaxValue(value);
            break;
        }
        nextPtr = nextPtr->next;
    }
}

void ProtocolDecoder::setDecodeTimeout(int ms) {
    mDecodeTimeout = ms;
}

void ProtocolDecoder::setDecodeErrorLogFlags(DecodeErrorFlags flags) {
    errorLogFlags = flags;
}

void ProtocolDecoder::enableMetaDataRecord() {
    metadataEnable = true;
}

void ProtocolDecoder::clearCache() {
    bufferCache.clear();
    lastDecodeTimestamp = 0;
}

ProtocolMetaData ProtocolDecoder::getLastMetaData() const {
    return lastMetaData;
}

ProtocolMetaData ProtocolDecoder::getLastMetaData(int type) const {
    return metaData[type];
}

PROTOCOL_CODEC_NAMESPACE_END
