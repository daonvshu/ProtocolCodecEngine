#include "protocolencoder.h"

#include "utils/byteutils.h"

#include "flagdata/dataaddress.h"
#include "flagdata/datacontent.h"
#include "flagdata/dataend.h"
#include "flagdata/dataheader.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"

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
    int contentSize = content.size();
    printInfo([&] {
        return QString("encode type %1 begin, content size: %2").arg(dataType).arg(content.size());
    });

    ProtocolMetaData encodeMetaData;
    auto next = protocolFlags;
    while (next != nullptr) {
        switch (next->flag) {
            case ProtocolFlag::Flag_Header: {
                encodeMetaData.header = encodeHeader(next);
                buffer.append(encodeMetaData.header);
                break;
            }
            case ProtocolFlag::Flag_Address: {
                encodeMetaData.address = encodeAddress(next);
                buffer.append(encodeMetaData.address);
                break;
            }
            case ProtocolFlag::Flag_Type: {
                encodeMetaData.type = encodeType(next, dataType);
                buffer.append(encodeMetaData.type);
                break;
            }
            case ProtocolFlag::Flag_Size: {
                encodeMetaData.size = encodeSize(next, contentSize);
                buffer.append(encodeMetaData.size);
                break;
            }
            case ProtocolFlag::Flag_Content: {
                encodeMetaData.content = content;
                buffer.append(content);
                break;
            }
            case ProtocolFlag::Flag_Verify: {
                encodeMetaData.verify = encodeVerify(next, buffer, contentSize);
                buffer.append(encodeMetaData.verify);
                break;
            }
            case ProtocolFlag::Flag_End: {
                encodeMetaData.end = encodeEnd(next);
                buffer.append(encodeMetaData.end);
                break;
            }
            default:
                break;
        }
        next = next->next;
    }
    metaData[dataType] = encodeMetaData;
    lastMetaData = encodeMetaData;

    return buffer;
}

ProtocolMetaData ProtocolEncoder::getLastMetaData() const {
    return lastMetaData;
}

ProtocolMetaData ProtocolEncoder::getLastMetaData(int type) const {
    return metaData[type];
}

QByteArray ProtocolEncoder::encodeHeader(ProtocolFlagData* flagData) {
    auto headerFlag = dynamic_cast<ProtocolFlagDataHeader*>(flagData);
    return headerFlag->target;
}

QByteArray ProtocolEncoder::encodeAddress(ProtocolFlagData* flagData) {
    auto addressFlag = dynamic_cast<ProtocolFlagDataAddress*>(flagData);
    auto sizeBuff = (char*) malloc(addressFlag->byteSize);
    memcpy(sizeBuff, &addressFlag->address, addressFlag->byteSize);
    if (!flagData->isLittleEndian) {
        ByteUtils::byteSwap(sizeBuff, addressFlag->byteSize);
    }
    QByteArray buff(sizeBuff, addressFlag->byteSize);
    free(sizeBuff);
    return buff;
}

QByteArray ProtocolEncoder::encodeType(ProtocolFlagData* flagData, int dataType) {
    QByteArray buff((char*)&dataType, flagData->byteSize);
    if (!flagData->isLittleEndian) {
        ByteUtils::byteSwap(buff);
    }
    return buff;
}

QByteArray ProtocolEncoder::encodeSize(ProtocolFlagData* flagData, int contentSize) {
    const ProtocolFlagData* header = flagData;
    while (header->prev != nullptr) {
        header = header->prev;
    }

    int byteSize = 0;
    auto next = header;
    while (next != nullptr) {
        if (next->isSizeTarget && next->flag != ProtocolFlag::Flag_Size) {
            byteSize += next->byteSize;
        }
        next = next->next;
    }
    byteSize += contentSize;

    QByteArray buff((char*)&byteSize, flagData->byteSize);
    if (!flagData->isLittleEndian) {
        ByteUtils::byteSwap(buff);
    }
    return buff;
}

QByteArray ProtocolEncoder::encodeVerify(ProtocolFlagData* flagData, const QByteArray& buffer, int contentSize) const {
    auto verifyFlag = dynamic_cast<ProtocolFlagDataVerify*>(flagData);
    auto verifyCode = verifyFlag->getVerifyCode(buffer, contentSize);
    printInfo([&] {
        return "content verify code: " + verifyCode.toHex(' ');
    });
    return verifyCode;
}

QByteArray ProtocolEncoder::encodeEnd(ProtocolFlagData* flagData) {
    auto endFlag = dynamic_cast<ProtocolFlagDataEnd*>(flagData);
    return endFlag->target;
}

PROTOCOL_CODEC_NAMESPACE_END
