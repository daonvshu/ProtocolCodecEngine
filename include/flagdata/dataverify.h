#pragma once

#include <qobject.h>

#include "global.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataVerify : public ProtocolFlagData {
public:
    enum VerifyType {
        Crc16,
        Sum8,
        Sum16,
    };

    explicit ProtocolFlagDataVerify(VerifyType verifyType);

    QString dataToString() override;

    bool verify(char *data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) override;

    void doFrameOffset(int &offset) override;

    void setVerifyFlags(const QList<QSharedPointer<ProtocolFlagData>>& flags);

    QByteArray getVerifyCode(const QByteArray& buff, int contentOffset);

    QSharedPointer<ProtocolFlagData> copy() const override;

public:
    VerifyType verifyType;
    int byteSize;

private:
    ProtocolFlagData* verifyFlags[256] = { nullptr };
};

PROTOCOL_CODEC_NAMESPACE_END