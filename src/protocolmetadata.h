#pragma once

#include "global.h"

#include <qbytearray.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

struct ProtocolMetaData {
    QByteArray header;
    QByteArray address;
    QByteArray type;
    QByteArray size;
    QByteArray content;
    QByteArray verify;
    QByteArray end;

    int decodeType = INT_MAX;
    QByteArray localVerifyCode;
};

PROTOCOL_CODEC_NAMESPACE_END