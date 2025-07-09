#include "flagdata/datasize.h"

#include <qloggingcategory.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN
    ProtocolFlagDataSize::ProtocolFlagDataSize(int byteSize)
    : ProtocolFlagData(ProtocolFlag::Flag_Size)
    , byteSize(byteSize)
    , dataSize(0)
    , sizeValueMax(-1)
{}

QString ProtocolFlagDataSize::dataToString() {
    return QString::number(byteSize);
}

bool ProtocolFlagDataSize::verify(char *data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) {
    if (offset + byteSize > maxSize) {
        return false;
    }
    //小端序
    dataSize = 0;
    for (int i = 0; i < byteSize; i++) {
        dataSize |= (data[offset + i] & 0xff) << (8 * i);
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

void ProtocolFlagDataSize::doFrameOffset(int &offset) {
    offset += byteSize;
}

QSharedPointer<ProtocolFlagData> ProtocolFlagDataSize::copy() const {
    auto newFlag = QSharedPointer<ProtocolFlagDataSize>::create(byteSize);
    newFlag->dataSize = dataSize;
    return newFlag;
}

void ProtocolFlagDataSize::setSizeMaxValue(int value) {
    sizeValueMax = value;
}

PROTOCOL_CODEC_NAMESPACE_END
