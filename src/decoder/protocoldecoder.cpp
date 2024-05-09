#include "decoder/protocoldecoder.h"

#include "flagdata/dataheader.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "flagdata/dataend.h"

#include <qdebug.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

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
}

void ProtocolDecoder::addType(int type, const std::function<void(const QByteArray &)> &decoder) {
    typeDecoders.insert(type, decoder);
}

void ProtocolDecoder::addBuffer(const QByteArray &buffer) {
    bufferCache.append(buffer);

    int dataOffset = 0;
    auto data = bufferCache.data();
    int dataSize = bufferCache.size();

    int decodedOffset = 0;

    while (dataOffset < dataSize) {
        int frameOffset = dataOffset;
        bool frameDecodeSuccess = true;
        for (const auto& flag : protocolFlags) {
            bool segmentValid = flag->verify(data, frameOffset, dataSize);
            if (segmentValid) {
                flag->doFrameOffset(frameOffset);
            } else {
                frameDecodeSuccess = false;
                break;
            }
        }
        if (!frameDecodeSuccess) {
            dataOffset++;
        } else {
            decodedOffset = frameOffset;
            dataOffset = frameOffset;

            auto content = get<ProtocolFlagDataContent>(ProtocolFlag::Flag_Content)->contentData;
            int type = 0;
            memcpy(&type, content.data(), 2);
            auto decoder = typeDecoders[type];
            if (!decoder) {
                qWarning() << "cannot find decoder to decode frame type:" << type;
            } else {
                decoder(content.mid(2));
            }
        }
    }

    if (decodedOffset != 0) {
        bufferCache = bufferCache.mid(decodedOffset);
    }
}

PROTOCOL_CODEC_NAMESPACE_END