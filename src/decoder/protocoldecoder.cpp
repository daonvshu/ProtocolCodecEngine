#include "protocoldecoder.h"

#include "flagdata/dataheader.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/datatype.h"

#include <qdatetime.h>

//#define DECODE_LOCAL_DEBUG
#ifdef DECODE_LOCAL_DEBUG
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qfile.h>
#endif

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
#ifdef DECODE_LOCAL_DEBUG
    //static int bufferAllSize = 0;
    //bufferAllSize -= bufferCache.size();
#endif

    if (mDecodeTimeout > 0) {
        auto cur = QDateTime::currentMSecsSinceEpoch();
        if (cur - lastDecodeTimestamp > mDecodeTimeout) {
            if (!bufferCache.isEmpty()) {
                printWarning([] {
                    return "decode timeout, clear buffer cache...";
                });
            }
            bufferCache.clear();
            validHeaderPos.clear();
        }
        lastDecodeTimestamp = cur;
    }

    bufferCache.append(buffer);

    int dataOffset = 0;
    auto data = bufferCache.data();
    int dataSize = bufferCache.size();

    int decodedOffset = 0;

#ifdef DECODE_LOCAL_DEBUG
    //QElapsedTimer loopTestTimer;
    //loopTestTimer.start();
    qDebug() << "decode buffer begin, current buffer size:" << bufferCache.size();
#endif

    ProtocolMetaData* decodeMetaData = nullptr;
    if (metadataEnable) {
        decodeMetaData = new ProtocolMetaData;
    }

    int lastCheckHeaderIndex = 0;
    int maxCheckHeaderSize = validHeaderPos.size();
    while (dataOffset < dataSize) {
        if (lastCheckHeaderIndex < maxCheckHeaderSize) {
            dataOffset = validHeaderPos[lastCheckHeaderIndex++];
        }
        int frameOffset = dataOffset;
        bool frameDecodeSuccess = true;
        for (int i = 0; i < flagCount; i++) {
            auto flag = decodeFlags[i];
            bool segmentValid = flag->verify(data, dataOffset, dataSize, decodeMetaData, debugPtr);
            if (segmentValid) {
                //offset used byte size
                if (flag->flag == ProtocolFlag::Flag_Content) {
                    frameOffset += dynamic_cast<ProtocolFlagDataContent*>(flag)->getByteSize();
                } else {
                    frameOffset += flag->byteSize;
                }
                if (flag->prev == nullptr) {
                    validHeaderPos << dataOffset;
                }
            } else {
                if (flag->flag == ProtocolFlag::Flag_Size) {
                    validHeaderPos.removeLast();
                }
                frameDecodeSuccess = false;
                break;
            }
        }

        if (decodeMetaData != nullptr && decodeMetaData->decodeType != INT_MAX) {
            metaData[decodeMetaData->decodeType] = *decodeMetaData;
            lastMetaData = *decodeMetaData;
        }

        if (!frameDecodeSuccess) {
            dataOffset++;
            continue;
        }

        decodedOffset = frameOffset;
        dataOffset = frameOffset;
        while (lastCheckHeaderIndex < maxCheckHeaderSize) {
            if (validHeaderPos[lastCheckHeaderIndex] < decodedOffset) {
                lastCheckHeaderIndex++;
            } else {
                break;
            }
        }

        int type = 0;
        QByteArray content;
        auto nextPtr = protocolFlags;
        while (nextPtr != nullptr && nextPtr->next != nullptr) {
            if (nextPtr->flag == ProtocolFlag::Flag_Type) {
                type = dynamic_cast<ProtocolFlagDataType*>(nextPtr)->value;
            } else if (nextPtr->flag == ProtocolFlag::Flag_Content) {
                content = dynamic_cast<ProtocolFlagDataContent*>(nextPtr)->contentData;
            }
            nextPtr = nextPtr->next;
        }
        if (metadataEnable) {
            metaData[type].content = content;
            lastMetaData.content = content;
        }

        auto decoder = typeDecoders[type];
        if (!decoder) {
            qWarning() << QString::asprintf("cannot find decoder to decode frame type: 0x%x", type);
        } else {
            decoder(content);
        }

#ifdef DECODE_LOCAL_DEBUG
        static qint64 lastDecodeTime = QDateTime::currentMSecsSinceEpoch();
        auto curTime = QDateTime::currentMSecsSinceEpoch();
        qDebug() << curTime - lastDecodeTime << " decode time, current loop:" << loopSize;
        lastDecodeTime = curTime;
#endif
    }

    delete decodeMetaData;

#ifdef DECODE_LOCAL_DEBUG
    //bufferAllSize += bufferCache.size();
    //qDebug() << "loop test time:" << loopTestTimer.elapsed() << "loop size:" << loopSize;
#endif

    validHeaderPos = validHeaderPos.mid(maxCheckHeaderSize);
    if (validHeaderPos.isEmpty()) {
        bufferCache = bufferCache.right(protocolFlags->byteSize);
        return;
    }
    decodedOffset = qMax(decodedOffset, validHeaderPos.first()); //如果第一个子对象匹配成功，则移除前置垃圾数据
    if (decodedOffset != 0) {
        bufferCache = bufferCache.mid(decodedOffset);
        for (auto& pos : validHeaderPos) {
            pos -= decodedOffset;
        }
    }
    if (mBufferMaxSize > 0) {
        if (bufferCache.size() > mBufferMaxSize) {
            int offset = bufferCache.size() - mBufferMaxSize;
            bufferCache = bufferCache.mid(offset);
            for (auto& pos : validHeaderPos) {
                pos -= offset;
            }
        }
    }
    for (int i = validHeaderPos.size() - 1; i >= 0; i--) {
        if (validHeaderPos[i] < 0) {
            validHeaderPos.removeAt(i);
        }
    }
    if (validHeaderPos.isEmpty()) {
        bufferCache = bufferCache.right(protocolFlags->byteSize);
    }

#ifdef DECODE_LOCAL_DEBUG
    qDebug() << "decode buffer end, cache size: " << bufferCache.size() << " loop size: "
    << loopSize << " header find size: " << headerFindSize << " decoded offset: " << decodedOffset
    << " header offset: " << validHeaderPos;
#endif
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

void ProtocolDecoder::enableMetaDataRecord() {
    metadataEnable = true;
}

void ProtocolDecoder::clearCache() {
    bufferCache.clear();
}

ProtocolMetaData ProtocolDecoder::getLastMetaData() const {
    return lastMetaData;
}

ProtocolMetaData ProtocolDecoder::getLastMetaData(int type) const {
    return metaData[type];
}

PROTOCOL_CODEC_NAMESPACE_END
