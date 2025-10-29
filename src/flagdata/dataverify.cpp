#include "dataverify.h"

#include <qloggingcategory.h>

#include "dataheader.h"
#include "datacontent.h"

#include "utils/byteutils.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

/**
 * @brief 和校验
 * @param arr
 * @param lens
 * @return char类型
 */
static uchar sumCheck(const char* arr, int lens) {
    uchar sum = 0;
    for (int i = 0; i < lens; i++) {
        sum += (uchar)*(arr + i);
    }
    return sum;
}

/**
 * @brief 和校验
 * @param arr
 * @param lens
 * @return short类型
 */
static ushort sumCheck2(const char* arr, int lens) {
    ushort sum = 0;
    for (int i = 0; i < lens; i++) {
        sum += (ushort)(*(arr + i) & 0xff);
    }
    return sum;
}

/**
 * @brief CRC16校验
 * @param data
 * @param length
 * @return
 */
static uint16_t crc16(uchar *data, int length) {
    int i;
    uint16_t crc_result = 0xffff;
    while (length--) {
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

ProtocolFlagDataVerify::ProtocolFlagDataVerify(VerifyType verifyType)
    : ProtocolFlagData(ProtocolFlag::Flag_Verify, 0)
    , verifyType(verifyType)
{
    switch (verifyType) {
        case Crc16:
            byteSize = 2;
            break;
        case Sum8:
            byteSize = 1;
            break;
        case Sum16:
            byteSize = 2;
            break;
        case None:
            byteSize = 0;
            break;
    }
}

QString ProtocolFlagDataVerify::dataToString() {
    switch (verifyType) {
        case Crc16: return "CRC16";
        case Sum8: return "SUM8";
        case Sum16: return "SUM16";
        case None: return "NONE";
    }
    return {};
}

bool ProtocolFlagDataVerify::verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) {

    int curDataOffset = baseOffset + getCurrentByteOffset();
    if (curDataOffset + byteSize > maxSize) {
        return false;
    }

    ProtocolFlagData* header = this;
    while (header->prev != nullptr) {
        header = header->prev;
    }

    QByteArray verifyTargetBytes;
    auto next = header;
    int byteOffset = 0;
    while (next != nullptr) {
        if (next->isVerifyTarget) {
            if (next->flag == ProtocolFlag::Flag_Content) {
                verifyTargetBytes.append(data + baseOffset + byteOffset, dynamic_cast<ProtocolFlagDataContent*>(next)->getByteSize());
            } else {
                verifyTargetBytes.append(next->getBytesContent());
            }
        }
        byteOffset += next->byteSize;
        next = next->next;
    }

    uint16_t curVerify = 0;
    switch (verifyType) {
        case Crc16: {
            curVerify = crc16(reinterpret_cast<uchar*>(verifyTargetBytes.data()), verifyTargetBytes.size());
            break;
        }
        case Sum8: {
            curVerify = sumCheck(verifyTargetBytes.data(), verifyTargetBytes.size());
            break;
        }
        case Sum16: {
            curVerify = sumCheck2(verifyTargetBytes.data(), verifyTargetBytes.size());
            break;
        }
        case None:
            return true;
    }

    uint16_t verifyCode = 0;
    memcpy(&verifyCode, data + curDataOffset, byteSize);
    if (!isLittleEndian) {
        ByteUtils::byteSwap(verifyCode, byteSize);
    }
    if (metaData) {
        metaData->verify = QByteArray(data + curDataOffset, byteSize);
        metaData->localVerifyCode = QByteArray((char*)&verifyCode, byteSize);
    }

    auto valid = curVerify == verifyCode;
    if (debugPtr && !valid) {
        QString verifyTypeStr;
        switch (verifyType) {
            case Crc16: verifyTypeStr = "CRC16"; break;
            case Sum8: verifyTypeStr = "SUM8"; break;
            case Sum16: verifyTypeStr = "SUM16"; break;
            case None: verifyTypeStr = "NONE"; break;
        }
        qCInfo(debugPtr) << QString("%1 check fail, calc code: %2, data code: %3")
            .arg(verifyTypeStr).arg(curVerify, 0, 16).arg(verifyCode, 0, 16);
    }
    return valid;
}

QByteArray ProtocolFlagDataVerify::getVerifyCode(const QByteArray &buff, int contentSize) {

    ProtocolFlagData* header = this;
    while (header->prev != nullptr) {
        header = header->prev;
    }

    QByteArray verifyTargetBytes;
    auto next = header;
    int byteOffset = 0;
    while (next != nullptr && byteOffset < buff.size()) {
        auto bytes = next->byteSize;
        if (next->flag == ProtocolFlag::Flag_Content) {
            bytes = contentSize;
        }
        if (next->isVerifyTarget) {
            verifyTargetBytes.append(buff.mid(byteOffset, bytes));
        }
        byteOffset += bytes;
        next = next->next;
    }

    uint16_t curVerify = 0;
    switch (verifyType) {
        case Crc16: {
            curVerify = crc16(reinterpret_cast<uchar*>(verifyTargetBytes.data()), verifyTargetBytes.size());
            break;
        }
        case Sum8: {
            curVerify = sumCheck(verifyTargetBytes.data(), verifyTargetBytes.size());
            break;
        }
        case Sum16: {
            curVerify = sumCheck2(verifyTargetBytes.data(), verifyTargetBytes.size());
            break;
        }
        case None:
            return QByteArray();
    }
    auto data = QByteArray((char*)&curVerify, byteSize);
    if (!isLittleEndian) {
        ByteUtils::byteSwap(data);
    }
    return data;
}

QByteArray ProtocolFlagDataVerify::getBytesContent() const {
    return QByteArray();
}

PROTOCOL_CODEC_NAMESPACE_END
