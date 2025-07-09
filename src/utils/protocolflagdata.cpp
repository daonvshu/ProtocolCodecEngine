#include "utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagData::ProtocolFlagData(ProtocolFlag flag)
    : flag(flag)
{}

QString ProtocolFlagData::dataToString() {
    return {};
}

PROTOCOL_CODEC_NAMESPACE_END



