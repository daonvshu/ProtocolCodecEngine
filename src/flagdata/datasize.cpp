#include "datasize.h"

#include "utils/byteutils.h"

#include <qloggingcategory.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN
    ProtocolFlagDataSize::ProtocolFlagDataSize(int byteSize)
    : ProtocolFlagData(ProtocolFlag::Flag_Size, byteSize)
    , dataSize(0)
    , sizeValueMax(-1)
{}

QString ProtocolFlagDataSize::dataToString() {
    return QString::number(byteSize);
}

bool ProtocolFlagDataSize::verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) {
    int curDataOffset = baseOffset + getCurrentByteOffset();
    if (curDataOffset + byteSize > maxSize) {
        return false;
    }

    dataSize = 0;
    memcpy(&dataSize, data + curDataOffset, byteSize);
    if (metaData) {
        metaData->size = QByteArray(data + curDataOffset, byteSize);
    }
    if (!isLittleEndian) {
        ByteUtils::byteSwap(dataSize, byteSize);
    }

    if (sizeValueMax > 0) {
        if (dataSize > sizeValueMax) {
            if (debugPtr) {
                qCInfo(debugPtr) << "data size greater than max value:" << dataSize;
            }
            return false;
        }
    }
    if (debugPtr && dataSize < 0) {
        qCInfo(debugPtr) << "data size error:" << dataSize;
    }
    return dataSize >= 0;
}

void ProtocolFlagDataSize::setSizeMaxValue(int value) {
    sizeValueMax = value;
}

int ProtocolFlagDataSize::getContentSize() const {
    const ProtocolFlagData* header = this;
    while (header->prev != nullptr) {
        header = header->prev;
    }

    int contentSize = dataSize;
    if (contentSize == 0) {
        return 0;
    }
    auto next = header;
    while (next != nullptr) {
        if (next->isSizeTarget && next->flag != ProtocolFlag::Flag_Size) {
            contentSize -= next->byteSize;
        }
        next = next->next;
    }
    return contentSize;
}

QByteArray ProtocolFlagDataSize::getBytesContent() const {
    auto bytes = QByteArray(reinterpret_cast<const char *>(&dataSize), byteSize);
    if (!isLittleEndian) {
        ByteUtils::byteSwap(bytes);
    }
    return bytes;
}

PROTOCOL_CODEC_NAMESPACE_END
