#include "sumcheckverify.h"

#include "utils/byteutils.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

SumCheckVerify::SumCheckVerify(int byteSize)
    : mByteSize(byteSize)
{}

void SumCheckVerify::getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) {
    if (mByteSize == 1) {
        uchar actual = sum8(sourceData.data(), sourceData.size());
        *resultBuffer = actual;
    }
    if (mByteSize == 2) {
        ushort actual = sum16(sourceData.data(), sourceData.size());
        if (!littleEndian) {
            ByteUtils::byteSwap(actual, mByteSize);
        }
        memcpy(resultBuffer, &actual, mByteSize);
    }
}

int SumCheckVerify::byteSize() const {
    return mByteSize;
}

uchar SumCheckVerify::sum8(const char* arr, int lens) {
    uchar sum = 0;
    for (int i = 0; i < lens; i++) {
        sum += (uchar)*(arr + i);
    }
    return sum;
}

ushort SumCheckVerify::sum16(const char* arr, int lens) {
    ushort sum = 0;
    for (int i = 0; i < lens; i++) {
        sum += (ushort)(*(arr + i) & 0xff);
    }
    return sum;
}

PROTOCOL_CODEC_NAMESPACE_END
