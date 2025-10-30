#pragma once

#include <qobject.h>

#include "protocolverify.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class NoneVerify : public ProtocolVerify {
public:
    void getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) override;
    int byteSize() const override;
};

PROTOCOL_CODEC_NAMESPACE_END