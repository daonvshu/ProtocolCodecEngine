#include "dataend.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN
ProtocolFlagDataEnd::ProtocolFlagDataEnd(QByteArray data)
    : ProtocolFlagData(ProtocolFlag::Flag_End, data.size())
    , target(std::move(data))
{}

QString ProtocolFlagDataEnd::dataToString() {
    return target.toHex(' ');
}

bool ProtocolFlagDataEnd::verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) {
    int curDataOffset = baseOffset + getCurrentByteOffset();
    if (curDataOffset + target.size() > maxSize) {
        return false;
    }
    if (metaData) {
        metaData->end = QByteArray(data + curDataOffset, target.size());
    }
    for (int i = 0; i < target.size(); i++) {
        if (data[i + curDataOffset] != target[i]) {
            return false;
        }
    }
    return true;
}

QByteArray ProtocolFlagDataEnd::getBytesContent() const {
    return target;
}

PROTOCOL_CODEC_NAMESPACE_END
