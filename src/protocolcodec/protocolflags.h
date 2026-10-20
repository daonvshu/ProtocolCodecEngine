#pragma once

#include "global.h"

#include <qdebug.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

enum class DecodeError {
    None = 0,
    HeaderMismatch = 1 << 0,
    AddressMismatch = 1 << 1,
    InvalidLength = 1 << 2,
    TailMismatch = 1 << 3,
    ChecksumMismatch = 1 << 4,
    UnknownType = 1 << 5,
    Timeout = 1 << 6,
    BufferOverflow = 1 << 7,
    All = (1 << 8) - 1,
};

Q_DECLARE_FLAGS(DecodeErrorFlags, DecodeError)
Q_DECLARE_OPERATORS_FOR_FLAGS(DecodeErrorFlags)

enum class ProtocolFlag {
    Flag_Header = 0,
    Flag_Address,
    Flag_Type,
    Flag_Size,
    Flag_Content,
    Flag_Verify,
    Flag_End,

    Flag_Max,
};

inline QDebug operator<<(QDebug debug, ProtocolFlag flag) {
    switch (flag) {
        case ProtocolFlag::Flag_Header:
            debug << "Flag_Header";
            break;
        case ProtocolFlag::Flag_Address:
            debug << "Flag_Address";
            break;
        case ProtocolFlag::Flag_Type:
            debug << "Flag_Type";
            break;
        case ProtocolFlag::Flag_Size:
            debug << "Flag_Size";
            break;
        case ProtocolFlag::Flag_Content:
            debug << "Flag_Content";
            break;
        case ProtocolFlag::Flag_Verify:
            debug << "Flag_Verify";
            break;
        case ProtocolFlag::Flag_End:
            debug << "Flag_End";
            break;
        default:
            break;
    }
    return debug;
}

PROTOCOL_CODEC_NAMESPACE_END
