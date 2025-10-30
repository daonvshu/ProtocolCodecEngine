#include "dataverify.h"

#include <qloggingcategory.h>

#include "dataheader.h"
#include "datacontent.h"

#include "utils/byteutils.h"

#include "verifier/sumcheckverify.h"
#include "verifier/crccheckverify.h"
#include "verifier/noneverify.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagDataVerify::ProtocolFlagDataVerify(VerifyType verifyType)
    : ProtocolFlagData(ProtocolFlag::Flag_Verify, 0)
    , verifyType(verifyType)
{
    switch (verifyType) {
        case Sum16:
            verifier = QSharedPointer<SumCheckVerify>::create(2);
            break;
        case Sum8:
            verifier = QSharedPointer<SumCheckVerify>::create(1);
            break;
        case Crc16:
            verifier = QSharedPointer<CrcCheckVerify>::create(2);
            break;
        case None:
            verifier = QSharedPointer<NoneVerify>::create();
            break;
        case Custom:
            break;
    }
    if (verifier) {
        byteSize = verifier->byteSize();
        decodeVerifyBuffer = new char[byteSize];
        encodeVerifyBuffer = new char[byteSize];
    } else {
        decodeVerifyBuffer = nullptr;
        encodeVerifyBuffer = nullptr;
    }
}

ProtocolFlagDataVerify::~ProtocolFlagDataVerify() {
    delete[] decodeVerifyBuffer;
    delete[] encodeVerifyBuffer;
}

void ProtocolFlagDataVerify::setCustomVerifier(const QSharedPointer<ProtocolVerify>& customVerifier) {
    verifier = customVerifier;
    byteSize = verifier->byteSize();

    delete[] decodeVerifyBuffer;
    decodeVerifyBuffer = new char[byteSize];
    delete[] encodeVerifyBuffer;
    encodeVerifyBuffer = new char[byteSize];
}

QString ProtocolFlagDataVerify::dataToString() {
    switch (verifyType) {
        case Crc16: return "CRC16";
        case Sum8: return "SUM8";
        case Sum16: return "SUM16";
        case Custom: return "CUSTOM";
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

    if (verifier.isNull() || byteSize == 0) {
        if (debugPtr) {
            qCWarning(debugPtr) << "Verifier is null!";
        }
        return false;
    }

    verifier->getVerifyCode(verifyTargetBytes, decodeVerifyBuffer, isLittleEndian);
    bool valid = true;
    for (int i = 0; i < byteSize; i++) {
        if (decodeVerifyBuffer[i] != data[curDataOffset + i]) {
            valid = false;
            break;
        }
    }
    if (metaData) {
        metaData->verify = QByteArray(data + curDataOffset, byteSize);
        metaData->localVerifyCode = QByteArray(decodeVerifyBuffer, byteSize);
    }

    if (debugPtr && !valid) {
        QString verifyTypeStr;
        switch (verifyType) {
            case Crc16: verifyTypeStr = "CRC16"; break;
            case Sum8: verifyTypeStr = "SUM8"; break;
            case Sum16: verifyTypeStr = "SUM16"; break;
            case Custom: verifyTypeStr = "CUSTOM"; break;
            case None: verifyTypeStr = "NONE"; break;
        }
        qCInfo(debugPtr) << QString("%1 check fail, calc code: %2, data code: %3")
            .arg(verifyTypeStr)
            .arg(QString(QByteArray((char*)&decodeVerifyBuffer, byteSize).toHex()))
            .arg(QString(QByteArray(data + curDataOffset, byteSize).toHex()));
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

    if (verifier.isNull() || byteSize == 0) {
        return QByteArray();
    }
    verifier->getVerifyCode(verifyTargetBytes, encodeVerifyBuffer, isLittleEndian);
    return QByteArray(encodeVerifyBuffer, byteSize);
}

QByteArray ProtocolFlagDataVerify::getBytesContent() const {
    return QByteArray();
}

PROTOCOL_CODEC_NAMESPACE_END
