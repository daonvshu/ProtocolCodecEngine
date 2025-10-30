#pragma once

#include <qbytearray.h>

#include <protocolcodec/global.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class PROTOCOL_CODEC_EXPORT ProtocolVerify {
public:
    virtual ~ProtocolVerify() = default;

    virtual void getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) = 0;
    virtual int byteSize() const = 0;
};

PROTOCOL_CODEC_NAMESPACE_END