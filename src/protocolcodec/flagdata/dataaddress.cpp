#include "dataaddress.h"

#include "utils/byteutils.h"

#include <qloggingcategory.h>

PROTOCOL_CODEC_USING_NAMESPACE

ProtocolFlagDataAddress::ProtocolFlagDataAddress(int byteSize)
    : ProtocolFlagData(ProtocolFlag::Flag_Address, byteSize)
    , address(0)
{
}

QString ProtocolFlagDataAddress::dataToString() {
    return QString::number(byteSize);
}

bool ProtocolFlagDataAddress::verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory &(*debugPtr)()) {
    int curDataOffset = baseOffset + getCurrentByteOffset();
    if (curDataOffset + byteSize > maxSize) {
        return false;
    }
    int dstAddress = 0;
    memcpy(&dstAddress, data + curDataOffset, byteSize);
    if (metaData) {
        metaData->address = QByteArray(data + curDataOffset, byteSize);
    }
    if (!isLittleEndian) {
        ByteUtils::byteSwap(dstAddress, byteSize);
    }
    if (dstAddress != address || dstAddress < 0) {
        if (debugPtr) {
            qWarning(debugPtr) << "address mismatch, dst address:" << dstAddress << " current address:" << address;
        }
        return false;
    }
    return true;
}

QByteArray ProtocolFlagDataAddress::getBytesContent() const {
    auto bytes = QByteArray(reinterpret_cast<const char *>(&address), byteSize);
    if (!isLittleEndian) {
        ByteUtils::byteSwap(bytes);
    }
    return bytes;
}
