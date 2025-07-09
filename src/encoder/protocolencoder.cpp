#include "encoder/protocolencoder.h"

#include "flagdata/dataheader.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "flagdata/dataend.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

void ProtocolEncoder::addType(int type, ProtocolTypeCodecInterface* codec) {
    typeEncoders.insert(type, codec);
}

ProtocolEncoder::~ProtocolEncoder() {
    qDeleteAll(typeEncoders);
}

ProtocolTypeCodecInterface *ProtocolEncoder::getCodec(int type) {
    return typeEncoders[type];
}

QByteArray ProtocolEncoder::encodeFrame(const QByteArray &content, int dataType) {
    QByteArray buffer;
    int contentOffset = 0;
    printInfo([&] {
        return QString("encode type %1 begin, content size: %2").arg(dataType).arg(content.size());
    });
    for (const auto& flag : protocolFlags) {
        switch (flag->flag) {
            case ProtocolFlag::Flag_Header: {
                auto headerFlag = qSharedPointerCast<ProtocolFlagDataHeader>(flag);
                buffer.append(headerFlag->target);
            }
                break;
            case ProtocolFlag::Flag_Size: {
                auto sizeFlag = qSharedPointerCast<ProtocolFlagDataSize>(flag);
                int contentSize = content.size() + mTypeByteSize;
                auto sizeBuff = (char*) malloc(sizeFlag->byteSize);
                for (int i = 0; i < sizeFlag->byteSize; i++) {
                    sizeBuff[i] = (char)((contentSize >> (i * 8)) & 0xff);
                }
                buffer.append(sizeBuff, sizeFlag->byteSize);
                sizeFlag->dataSize = contentSize;
                free(sizeBuff);
            }
                break;
            case ProtocolFlag::Flag_Content: {
                contentOffset = buffer.size();
                buffer.append((char*)&dataType, mTypeByteSize).append(content);
            }
                break;
            case ProtocolFlag::Flag_Verify: {
                auto verifyFlag = qSharedPointerCast<ProtocolFlagDataVerify>(flag);
                auto verifyCode = verifyFlag->getVerifyCode(buffer, contentOffset);
                printInfo([&] {
                    return "content verify code: " + verifyCode.toHex(' ');
                });
                buffer.append(verifyCode);
            }
                break;
            case ProtocolFlag::Flag_End: {
                auto endFlag = qSharedPointerCast<ProtocolFlagDataEnd>(flag);
                buffer.append(endFlag->target);
            }
                break;
            default:
                break;
        }
    }
    return buffer;
}


PROTOCOL_CODEC_NAMESPACE_END