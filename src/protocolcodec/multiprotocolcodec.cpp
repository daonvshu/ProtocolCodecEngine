#include "multiprotocolcodec.h"

#include "protocolexception.h"
#include "flagdata/dataaddress.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "utils/protocoltemplatedecoder.h"

#include <QElapsedTimer>
#include <QQueue>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QVector>
#include <array>
#include <climits>
#include <cstring>
#include <vector>

PROTOCOL_CODEC_NAMESPACE_BEGIN

struct MultiProtocolCodec::Private {
    struct Entry {
        int id;
        quint32 address = 0;
        bool encodeOnly = false;
        QByteArray header;
        QByteArray tail;
        QString shape;
        QMap<int, int> lengths;
        ProtocolCodecEngine codec;
    };

    std::vector<std::unique_ptr<Entry>> entries;
    QHash<int, Entry*> ids;
    std::array<QVector<Entry*>, 256> headers;
    QByteArray buffer;
    QQueue<QByteArray> incoming;
    ProtocolDecoder diagnostics;
    QElapsedTimer clock;
    qint64 lastReceive = -1;
    int bufferMaxSize = 10 * 1024 * 1024;
    int timeout = -1;
    int lastProtocolId = -1;
    bool processing = false;
    bool clearRequested = false;
    bool metadata = false;
    bool hasDecodeProtocols = false;
    LoggingCategoryPtr logging = nullptr;
    DecodeErrorFlags errorFlags = DecodeError::AddressMismatch | DecodeError::InvalidLength
        | DecodeError::ChecksumMismatch | DecodeError::UnknownType | DecodeError::Timeout;
};

MultiProtocolCodec::MultiProtocolCodec(QObject* parent)
    : QObject(parent), d(new Private) {
    d->clock.start();
}

MultiProtocolCodec::~MultiProtocolCodec() = default;

void MultiProtocolCodec::addProtocol(int protocolId, const QString& frameTemplate) {
    ProtocolDefinition definition;
    definition.frameTemplate = frameTemplate;
    addProtocol(protocolId, definition);
}

void MultiProtocolCodec::addProtocol(int protocolId, const ProtocolDefinition& definition) {
    if (d->processing || !d->buffer.isEmpty() || !d->incoming.isEmpty()) {
        throw ProtocolException("Clear pending input before changing multi-protocol configuration.");
    }
    if (protocolId < 0 || d->ids.contains(protocolId)) {
        throw ProtocolException("Protocol ID must be nonnegative and unique.");
    }
    // Validate the complete grammar before allocating the parser's linked field list.
    static const QRegularExpression syntax(QStringLiteral(
        "\\A(?:H\\((?:[0-9A-Fa-f]{2})+\\)|E\\((?:[0-9A-Fa-f]{2})+\\)|"
        "[ATS][1-4]!?|C[0-9]*|V!?\\((?:CRC16|SUM8|SUM16)\\))+\\z"));
    if (!syntax.match(definition.frameTemplate).hasMatch()) {
        throw ProtocolException("Invalid multi-protocol frame definition.");
    }
    static const QRegularExpression fixedContent(QStringLiteral(
        "[HE]\\([0-9A-Fa-f]+\\)|V!?\\((?:CRC16|SUM8|SUM16)\\)|[ATS][1-4]!?|C([0-9]*)"));
    auto contentTokens = fixedContent.globalMatch(definition.frameTemplate);
    while (contentTokens.hasNext()) {
        const auto fixedMatch = contentTokens.next();
        if (fixedMatch.captured(1).isEmpty()) continue;
        bool valid = false;
        fixedMatch.captured(1).toInt(&valid);
        if (!valid) throw ProtocolException("Fixed content length exceeds the supported range.");
    }

    std::unique_ptr<Private::Entry> entry(new Private::Entry);
    entry->id = protocolId;
    entry->encodeOnly = definition.encodeOnly;
    entry->codec.flags = ProtocolTemplateDecoder::parse(definition.frameTemplate);
    QMap<ProtocolFlag, ProtocolFlagData*> fields;
    for (auto* field = entry->codec.flags; field != nullptr; field = field->next) {
        if (fields.contains(field->flag)) throw ProtocolException("Repeated protocol fields are unsupported.");
        fields.insert(field->flag, field);
    }
    if (entry->codec.flags->flag != ProtocolFlag::Flag_Header || !fields.contains(ProtocolFlag::Flag_Type)) {
        throw ProtocolException("Multi-protocol frames require a leading Header and an explicit Type.");
    }
    auto* content = static_cast<ProtocolFlagDataContent*>(fields.value(ProtocolFlag::Flag_Content));
    auto* size = fields.value(ProtocolFlag::Flag_Size);
    auto* verify = fields.value(ProtocolFlag::Flag_Verify);
    if (content != nullptr) {
        content->isSizeTarget = size != nullptr;
        content->isVerifyTarget = verify != nullptr;
        if (content->isFixByteSize && size != nullptr) {
            throw ProtocolException("Use fixed Content or Size-based Content, not both.");
        }
        if (!content->isFixByteSize) {
            const auto source = size != nullptr ? ProtocolFlag::Flag_Size : ProtocolFlag::Flag_Type;
            auto* previous = content->prev;
            while (previous != nullptr && previous->flag != source) previous = previous->prev;
            if (previous == nullptr) throw ProtocolException("Content requires a preceding length source.");
            if (size == nullptr && definition.contentSizeFromType.isEmpty()) {
                throw ProtocolException("Content without Size requires a type-length table.");
            }
        }
    }
    for (auto value : definition.contentSizeFromType) {
        if (value < 0) throw ProtocolException("Content lengths must be nonnegative.");
    }
    const auto validateTargets = [&](const QString& targets) {
        const QString names = QStringLiteral("HATSCVE");
        for (const QChar target : targets) {
            const int index = names.indexOf(target);
            if (index < 0 || !fields.contains(static_cast<ProtocolFlag>(index))) {
                throw ProtocolException("A selected field is missing from the frame definition.");
            }
        }
    };
    validateTargets(definition.sizeFlags);
    validateTargets(definition.verifyFlags);
    if (size == nullptr && definition.sizeFlags.contains('C')) {
        throw ProtocolException("Content cannot depend on a missing Size field.");
    }
    if (definition.verifyFlags.contains('V')) {
        throw ProtocolException("A checksum cannot include itself.");
    }
    entry->codec.setSizeFlags(definition.sizeFlags);
    entry->codec.setVerifyFlags(definition.verifyFlags);
    if (content != nullptr) content->setContentSizeFromType(definition.contentSizeFromType);
    bool passedVerifier = false;
    for (auto* field = entry->codec.flags; field != nullptr; field = field->next) {
        if (field->flag == ProtocolFlag::Flag_Verify) passedVerifier = true;
        else if (passedVerifier && field->isVerifyTarget) {
            throw ProtocolException("Verify must follow every selected checksum field.");
        }
        entry->shape += QString("%1:%2:%3:%4:%5;").arg(int(field->flag)).arg(field->byteSize)
            .arg(field->isLittleEndian).arg(field->isSizeTarget).arg(field->isVerifyTarget);
        if (field->flag == ProtocolFlag::Flag_Verify) {
            entry->shape += QString::number(int(static_cast<ProtocolFlagDataVerify*>(field)->verifyType));
        }
    }
    if (content != nullptr) {
        entry->shape += content->isFixByteSize ? "fixed" : "variable";
        if (!content->isFixByteSize && size == nullptr) entry->lengths = definition.contentSizeFromType;
    }
    entry->header = entry->codec.flags->getBytesContent();
    if (auto* tail = fields.value(ProtocolFlag::Flag_End)) entry->tail = tail->getBytesContent();
    if (auto* address = fields.value(ProtocolFlag::Flag_Address)) {
        const quint64 limit = (quint64(1) << (address->byteSize * 8)) - 1;
        if (definition.address > limit || definition.address > quint32(INT_MAX)) {
            throw ProtocolException("Address does not fit its configured field.");
        }
        entry->address = definition.address;
        static_cast<ProtocolFlagDataAddress*>(address)->address = definition.address;
    }
    for (const auto& existing : d->entries) {
        if (existing->shape != entry->shape || existing->lengths != entry->lengths) {
            throw ProtocolException("All protocols must have the same field structure and length rules.");
        }
        if (!existing->encodeOnly && !entry->encodeOnly
            && existing->header == entry->header && existing->tail == entry->tail
            && existing->address == entry->address) {
            throw ProtocolException("Protocols require distinct Header, End or Address features.");
        }
    }
    if (size != nullptr) static_cast<ProtocolFlagDataSize*>(size)->setSizeMaxValue(definition.maximumSize);
    entry->codec.decoder.setFlags(entry->codec.flags);
    entry->codec.encoder.setFlags(entry->codec.flags);
    entry->codec.setLogging(d->logging);
    entry->codec.setDecodeErrorLogFlags(d->errorFlags);
    if (d->metadata) entry->codec.enableRecordDecodeMetaData();
    auto* stored = entry.get();
    d->entries.push_back(std::move(entry));
    d->ids.insert(protocolId, stored);
    if (!stored->encodeOnly) {
        d->headers[static_cast<unsigned char>(stored->header.at(0))].append(stored);
        d->hasDecodeProtocols = true;
    }
}

QList<int> MultiProtocolCodec::protocolIds() const {
    QList<int> result;
    for (const auto& entry : d->entries) result.append(entry->id);
    return result;
}

ProtocolCodecEngine& MultiProtocolCodec::requireProtocol(int protocolId) const {
    auto* entry = d->ids.value(protocolId, nullptr);
    if (entry == nullptr) throw ProtocolException("Unknown protocol ID.");
    return entry->codec;
}

void MultiProtocolCodec::clearProtocols() {
    if (d->processing) throw ProtocolException("Cannot clear protocols inside a decode callback.");
    clearDecodeCacheBuffer();
    d->ids.clear();
    for (auto& bucket : d->headers) bucket.clear();
    d->entries.clear();
    d->hasDecodeProtocols = false;
    d->lastProtocolId = -1;
}

void MultiProtocolCodec::appendBuffer(const QByteArray& buffer) {
    if (buffer.isEmpty() || !d->hasDecodeProtocols) return;
    d->incoming.enqueue(buffer);
    if (d->processing) return;
    QScopedValueRollback<bool> processing(d->processing, true);
    while (!d->incoming.isEmpty()) {
        const auto input = d->incoming.dequeue();
        const auto now = d->clock.elapsed();
        if (d->timeout > 0 && d->lastReceive >= 0 && now - d->lastReceive > d->timeout
            && !d->buffer.isEmpty()) {
            d->diagnostics.logDecodeError(DecodeError::Timeout, "receive gap exceeded timeout; discarded cached bytes",
                d->buffer, false);
            d->buffer.clear();
        }
        d->lastReceive = now;
        if (input.size() > INT_MAX - d->buffer.size()) {
            throw ProtocolException("Receive batch exceeds the supported buffer range.");
        }
        d->buffer.append(input);
        processBuffer();
    }
}

void MultiProtocolCodec::processBuffer() {
    int offset = 0;
    int noiseStart = 0;
    const auto logNoise = [&](int end) {
        if (end > noiseStart && d->diagnostics.errorLoggingEnabled(DecodeError::HeaderMismatch)) {
            d->diagnostics.logDecodeError(DecodeError::HeaderMismatch, "discarded bytes before a matching header",
                d->buffer.mid(noiseStart, end - noiseStart), false);
        }
    };
    while (offset < d->buffer.size()) {
        const auto& bucket = d->headers[static_cast<unsigned char>(d->buffer.at(offset))];
        if (bucket.isEmpty()) {
            ++offset;
            continue;
        }
        Private::Entry* accepted = nullptr;
        Private::Entry* rejected = nullptr;
        ProtocolMetaData acceptedMetadata;
        QByteArray acceptedContent;
        int acceptedSize = 0;
        int acceptedType = INT_MAX;
        int rejectedSize = -1;
        DecodeError rejectedError = DecodeError::None;
        QString rejectedDetail;
        bool incomplete = false;
        bool matched = false;
        for (auto* entry : bucket) {
            const int available = d->buffer.size() - offset;
            const int prefixSize = qMin(available, int(entry->header.size()));
            if (std::memcmp(d->buffer.constData() + offset, entry->header.constData(), prefixSize) != 0) continue;
            matched = true;
            if (prefixSize < entry->header.size()) {
                incomplete = true;
                continue;
            }
            int frameSize = -1;
            int type = INT_MAX;
            QByteArray content;
            ProtocolMetaData metadata;
            DecodeError error = DecodeError::None;
            QString detail;
            auto result = entry->codec.decoder.decodeFrame(d->buffer, offset,
                d->metadata ? &metadata : nullptr, frameSize, type, content, error, detail);
            if (d->bufferMaxSize > 0 && frameSize > d->bufferMaxSize) {
                result = ProtocolDecoder::DecodeResult::Invalid;
                error = DecodeError::InvalidLength;
                detail = "declared frame exceeds shared buffer limit";
            }
            if (result == ProtocolDecoder::DecodeResult::Complete) {
                accepted = entry;
                acceptedSize = frameSize;
                acceptedType = type;
                acceptedContent = content;
                acceptedMetadata = metadata;
                break;
            }
            if (result == ProtocolDecoder::DecodeResult::Incomplete) incomplete = true;
            else if (rejected == nullptr) {
                rejected = entry;
                rejectedSize = frameSize;
                rejectedError = error;
                rejectedDetail = detail;
            }
        }
        if (!matched) {
            ++offset;
            continue;
        }
        logNoise(offset);
        noiseStart = offset;
        if (accepted != nullptr) {
            auto& decoder = accepted->codec.decoder;
            if (d->metadata) {
                decoder.metaData[acceptedType] = acceptedMetadata;
                decoder.lastMetaData = acceptedMetadata;
            }
            d->lastProtocolId = accepted->id;
            offset += acceptedSize;
            noiseStart = offset;
            const auto callback = decoder.typeDecoders.value(acceptedType);
            try {
                if (callback) callback(acceptedContent);
                else decoder.logDecodeError(DecodeError::UnknownType,
                    QString("protocol=%1 no decoder registered for type=0x%2")
                        .arg(accepted->id).arg(quint32(acceptedType), 0, 16),
                    d->buffer.mid(offset - acceptedSize, acceptedSize), true);
            } catch (...) {
                if (d->clearRequested) {
                    d->buffer.clear();
                    d->clearRequested = false;
                } else d->buffer.remove(0, offset);
                throw;
            }
            if (d->clearRequested) {
                d->buffer.clear();
                d->clearRequested = false;
                return;
            }
            continue;
        }
        // Do not interpret a valid inner frame while its outer candidate is incomplete.
        if (incomplete) break;
        if (rejected != nullptr && rejected->codec.decoder.errorLoggingEnabled(rejectedError)) {
            const bool complete = rejectedSize > 0 && rejectedSize <= d->buffer.size() - offset;
            rejected->codec.decoder.logDecodeError(rejectedError,
                QString("protocol=%1 ").arg(rejected->id) + rejectedDetail,
                complete ? d->buffer.mid(offset, rejectedSize) : d->buffer.mid(offset), complete);
        }
        noiseStart = ++offset;
    }
    if (offset == d->buffer.size()) logNoise(offset);
    d->buffer.remove(0, offset);
    if (d->bufferMaxSize > 0 && d->buffer.size() > d->bufferMaxSize) {
        const int discarded = d->buffer.size() - d->bufferMaxSize;
        d->diagnostics.logDecodeError(DecodeError::BufferOverflow, "shared receive buffer exceeded limit",
            d->buffer.left(discarded), false);
        d->buffer.remove(0, discarded);
    }
}

void MultiProtocolCodec::clearDecodeCacheBuffer() {
    d->incoming.clear();
    d->lastReceive = -1;
    if (d->processing) d->clearRequested = true;
    else d->buffer.clear();
}

void MultiProtocolCodec::setBufferMaxSize(int size) { d->bufferMaxSize = size; }
void MultiProtocolCodec::setDecodeTimeout(int milliseconds) { d->timeout = milliseconds; }

void MultiProtocolCodec::setLogging(LoggingCategoryPtr category) {
    d->logging = category;
    d->diagnostics.setLogging(category);
    for (const auto& entry : d->entries) entry->codec.setLogging(category);
}

void MultiProtocolCodec::setDecodeErrorLogFlags(DecodeErrorFlags flags) {
    d->errorFlags = flags;
    d->diagnostics.setDecodeErrorLogFlags(flags);
    for (const auto& entry : d->entries) entry->codec.setDecodeErrorLogFlags(flags);
}

void MultiProtocolCodec::enableRecordDecodeMetaData() {
    d->metadata = true;
    for (const auto& entry : d->entries) entry->codec.enableRecordDecodeMetaData();
}

int MultiProtocolCodec::lastDecodedProtocolId() const { return d->lastProtocolId; }
ProtocolMetaData MultiProtocolCodec::getLastDecodeMetaData(int protocolId) const {
    return requireProtocol(protocolId).getLastDecodeMetaData();
}
ProtocolMetaData MultiProtocolCodec::getLastDecodeMetaData(int protocolId, int type) const {
    return requireProtocol(protocolId).getLastDecodeMetaData(type);
}
ProtocolMetaData MultiProtocolCodec::getLastEncodeMetaData(int protocolId) const {
    return requireProtocol(protocolId).getLastEncodeMetaData();
}

PROTOCOL_CODEC_NAMESPACE_END
