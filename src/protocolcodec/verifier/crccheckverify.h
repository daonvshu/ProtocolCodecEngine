#pragma once

#include <qobject.h>

#include "protocolverify.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class CrcCheckVerify : public ProtocolVerify {
public:
    explicit CrcCheckVerify(int byteSize);

    void getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) override;
    int byteSize() const override;

private:
    int mByteSize;

private:
    static ushort crc16(uchar* data, int len);
};

PROTOCOL_CODEC_NAMESPACE_END