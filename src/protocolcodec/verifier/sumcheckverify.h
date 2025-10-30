#pragma once

#include <qobject.h>

#include "protocolverify.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class SumCheckVerify : public ProtocolVerify {
public:
    explicit SumCheckVerify(int byteSize);

    void getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) override;
    int byteSize() const override;

private:
    int mByteSize;

private:
    static uchar sum8(const char* arr, int lens);
    static ushort sum16(const char* arr, int lens);
};

PROTOCOL_CODEC_NAMESPACE_END