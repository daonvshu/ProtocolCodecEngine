#include "decoder/protocoldecoder.h"

#include "flagdata/dataheader.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "flagdata/dataend.h"

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

void ProtocolDecoder::setFlags(const QList<QSharedPointer<ProtocolFlagData>> &flags) {
    ProtocolCodecInterface::setFlags(flags);

    static QMap<ProtocolFlag, int> flagWeight = {
            { ProtocolFlag::Flag_Header, 100 },
            { ProtocolFlag::Flag_Size, 99 },
            { ProtocolFlag::Flag_End, 98 },
            { ProtocolFlag::Flag_Verify, 97 },
            { ProtocolFlag::Flag_Content, 96 },
    };
    std::sort(protocolFlags.begin(), protocolFlags.end(), [] (const QSharedPointer<ProtocolFlagData>& a, const QSharedPointer<ProtocolFlagData>& b) {
        return flagWeight[a->flag] > flagWeight[b->flag];
    });

    flagCount = protocolFlags.size();
    for (int i = 0; i < flagCount; i++) {
        decodeFlags[i] = protocolFlags[i].data();
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
            bool segmentValid = flag->verify(data, frameOffset, dataSize, debugPtr);
            if (segmentValid) {
                flag->doFrameOffset(frameOffset);
                if (flag->flag == ProtocolFlag::Flag_Header) {
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
        if (!frameDecodeSuccess) {
            dataOffset++;
        } else {
            decodedOffset = frameOffset;
            dataOffset = frameOffset;
            while (lastCheckHeaderIndex < maxCheckHeaderSize) {
                if (validHeaderPos[lastCheckHeaderIndex] < decodedOffset) {
                    lastCheckHeaderIndex++;
                } else {
                    break;
                }
            }

            auto content = get<ProtocolFlagDataContent>(ProtocolFlag::Flag_Content)->contentData;
            int type = 0;
            memcpy(&type, content.data(), mTypeByteSize);
            auto decoder = typeDecoders[type];
            if (!decoder) {
                qWarning() << QString::asprintf("cannot find decoder to decode frame type: 0x%x", type);
            } else {
                decoder(content.mid(mTypeByteSize));
            }

#ifdef DECODE_LOCAL_DEBUG
            static qint64 lastDecodeTime = QDateTime::currentMSecsSinceEpoch();
            auto curTime = QDateTime::currentMSecsSinceEpoch();
            qDebug() << curTime - lastDecodeTime << " decode time, current loop:" << loopSize;
            lastDecodeTime = curTime;
#endif
        }
    }

#ifdef DECODE_LOCAL_DEBUG
    //bufferAllSize += bufferCache.size();
    //qDebug() << "loop test time:" << loopTestTimer.elapsed() << "loop size:" << loopSize;
#endif

    validHeaderPos = validHeaderPos.mid(maxCheckHeaderSize);
    if (validHeaderPos.isEmpty()) {
        auto header = get<ProtocolFlagDataHeader>(ProtocolFlag::Flag_Header);
        if (!header) {
            bufferCache.clear();
        } else {
            bufferCache = bufferCache.right(header->target.size());
        }
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
        auto header = get<ProtocolFlagDataHeader>(ProtocolFlag::Flag_Header);
        if (!header) {
            bufferCache.clear();
        } else {
            bufferCache = bufferCache.right(header->target.size());
        }
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

void ProtocolDecoder::setSizeMaxValue(int value) {
    get<ProtocolFlagDataSize>(ProtocolFlag::Flag_Size)->setSizeMaxValue(value);
}

void ProtocolDecoder::setDecodeTimeout(int ms) {
    mDecodeTimeout = ms;
}

PROTOCOL_CODEC_NAMESPACE_END