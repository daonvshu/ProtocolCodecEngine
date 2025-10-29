#include "dataheader.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagDataHeader::ProtocolFlagDataHeader(QByteArray data)
    : ProtocolFlagData(ProtocolFlag::Flag_Header, data.size())
    , target(std::move(data))
{}

QString ProtocolFlagDataHeader::dataToString() {
    return target.toHex(' ');
}

bool ProtocolFlagDataHeader::verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) {
    int curDataOffset = baseOffset + getCurrentByteOffset();
    if (curDataOffset + target.size() > maxSize) {
        return false;
    }
    if (metaData) {
        metaData->header = QByteArray(data + curDataOffset, target.size());
    }
    for (int i = 0; i < target.size(); i++) {
        if (data[i + curDataOffset] != target[i]) {
            return false;
        }
    }
    return true;
}

QByteArray ProtocolFlagDataHeader::getBytesContent() const {
    return target;
}

PROTOCOL_CODEC_NAMESPACE_END
