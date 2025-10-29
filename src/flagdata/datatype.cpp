#include "datatype.h"

#include "utils/byteutils.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagDataType::ProtocolFlagDataType(int byteSize)
    : ProtocolFlagData(ProtocolFlag::Flag_Type, byteSize)
    , value(0)
{
}

QString ProtocolFlagDataType::dataToString() {
    return QString::number(value);
}

bool ProtocolFlagDataType::verify(char* data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory&(* debugPtr)()) {
    int curDataOffset = baseOffset + getCurrentByteOffset();
    if (curDataOffset + byteSize > maxSize) {
        return false;
    }
    value = 0;
    memcpy(&value, data + curDataOffset, byteSize);
    if (!isLittleEndian) {
        ByteUtils::byteSwap(value, byteSize);
    }

    if (metaData) {
        metaData->type = QByteArray(data + curDataOffset, byteSize);
        metaData->decodeType = value;
    }

    return true;
}

QByteArray ProtocolFlagDataType::getBytesContent() const {
    auto bytes = QByteArray(reinterpret_cast<const char *>(&value), byteSize);
    if (!isLittleEndian) {
        ByteUtils::byteSwap(bytes);
    }
    return bytes;
}

PROTOCOL_CODEC_NAMESPACE_END
