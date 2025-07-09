#include "flagdata/dataheader.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagDataHeader::ProtocolFlagDataHeader(QByteArray data)
    : ProtocolFlagData(ProtocolFlag::Flag_Header)
    , target(std::move(data))
{}

QString ProtocolFlagDataHeader::dataToString() {
    return target.toHex(' ');
}

bool ProtocolFlagDataHeader::verify(char *data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) {
    if (offset + target.size() > maxSize) {
        return false;
    }
    for (int i = 0; i < target.size(); i++) {
        if (data[i + offset] != target[i]) {
            return false;
        }
    }
    return true;
}

void ProtocolFlagDataHeader::doFrameOffset(int &offset) {
    offset += target.size();
}

QSharedPointer<ProtocolFlagData> ProtocolFlagDataHeader::copy() const {
    return QSharedPointer<ProtocolFlagDataHeader>::create(target);
}

PROTOCOL_CODEC_NAMESPACE_END