#pragma once

#include <qobject.h>
#include <qsharedpointer.h>

#include <protocolcodec/global.h>
#include <protocolcodec/utils/protocolflagdata.h>
#include <protocolcodec/verifier/protocolverify.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataVerify : public ProtocolFlagData {
public:
    enum VerifyType {
        Crc16,
        Sum8,
        Sum16,
        Custom,
        None,
    };

    explicit ProtocolFlagDataVerify(VerifyType verifyType = None);
    ~ProtocolFlagDataVerify() override;

    void setCustomVerifier(const QSharedPointer<ProtocolVerify>& customVerifier);

    QString dataToString() override;

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) override;

    QByteArray getVerifyCode(const QByteArray& buff, int contentSize);

    QByteArray getBytesContent() const override;

public:
    VerifyType verifyType;
    QSharedPointer<ProtocolVerify> verifier;
    char* decodeVerifyBuffer;
    char* encodeVerifyBuffer;
};

PROTOCOL_CODEC_NAMESPACE_END