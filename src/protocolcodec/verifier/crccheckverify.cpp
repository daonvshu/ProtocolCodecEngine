#include "crccheckverify.h"

#include "utils/byteutils.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

CrcCheckVerify::CrcCheckVerify(int byteSize)
    : mByteSize(byteSize)
{}

void CrcCheckVerify::getVerifyCode(const QByteArray& sourceData, char* resultBuffer, bool littleEndian) {
    if (mByteSize == 2) {
        ushort actual = crc16((uchar*)sourceData.data(), sourceData.size());
        if (!littleEndian) {
            ByteUtils::byteSwap(actual, mByteSize);
        }
        memcpy(resultBuffer, &actual, mByteSize);
    }
}

int CrcCheckVerify::byteSize() const {
    return mByteSize;
}

ushort CrcCheckVerify::crc16(uchar* data, int len) {
    int i;
    ushort crc_result = 0xffff;
    while (len--) {
        crc_result ^= (*data++) & 0xff;
        for (i = 0; i < 8; i++) {
            if (crc_result & 0x01)
                crc_result = (crc_result >> 1) ^ 0xa001;
            else
                crc_result = crc_result >> 1;
        }
    }
    return crc_result;
}

PROTOCOL_CODEC_NAMESPACE_END
