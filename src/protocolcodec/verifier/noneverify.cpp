#include "noneverify.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

void NoneVerify::getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) {
}

int NoneVerify::byteSize() const {
    return 0;
}

PROTOCOL_CODEC_NAMESPACE_END
