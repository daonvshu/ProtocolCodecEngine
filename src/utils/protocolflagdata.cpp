#include "utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagData::ProtocolFlagData(ProtocolFlag flag)
    : flag(flag)
{}

QString ProtocolFlagData::dataToString() {
    return {};
}

ProtocolFlagDataHeader::ProtocolFlagDataHeader(QByteArray data)
    : ProtocolFlagData(ProtocolFlag::Flag_Header)
    , data(std::move(data))
{}

QString ProtocolFlagDataHeader::dataToString() {
    return data.toHex(' ');
}

ProtocolFlagDataSize::ProtocolFlagDataSize(int size)
    : ProtocolFlagData(ProtocolFlag::Flag_Size)
    , size(size)
{}

QString ProtocolFlagDataSize::dataToString() {
    return QString::number(size);
}

ProtocolFlagDataContent::ProtocolFlagDataContent()
    : ProtocolFlagData(ProtocolFlag::Flag_Content)
{}

ProtocolFlagDataVerify::ProtocolFlagDataVerify(ProtocolFlagDataVerify::VerifyType verifyType)
    : ProtocolFlagData(ProtocolFlag::Flag_Verify)
    , verifyType(verifyType)
{}

QString ProtocolFlagDataVerify::dataToString() {
    switch (verifyType) {
        case Crc: return "CRC";
        case Sum: return "SUM";
    }
    return {};
}

ProtocolFlagDataEnd::ProtocolFlagDataEnd(QByteArray data)
    : ProtocolFlagData(ProtocolFlag::Flag_End)
    , data(std::move(data))
{}

QString ProtocolFlagDataEnd::dataToString() {
    return data.toHex(' ');
}

PROTOCOL_CODEC_NAMESPACE_END



