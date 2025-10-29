#pragma once

#include <qobject.h>

#include "../global.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataVerify : public ProtocolFlagData {
public:
    enum VerifyType {
        Crc16,
        Sum8,
        Sum16,
        None,
    };

    explicit ProtocolFlagDataVerify(VerifyType verifyType = None);

    QString dataToString() override;

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) override;

    QByteArray getVerifyCode(const QByteArray& buff, int contentSize);

    QByteArray getBytesContent() const override;

public:
    VerifyType verifyType;
};

PROTOCOL_CODEC_NAMESPACE_END