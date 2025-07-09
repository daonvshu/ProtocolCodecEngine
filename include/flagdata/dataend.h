#pragma once

#include <qobject.h>

#include "global.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataSize;
class ProtocolFlagDataVerify;
class ProtocolFlagDataContent;

class ProtocolFlagDataEnd : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataEnd(QByteArray data);

    QString dataToString() override;

    bool verify(char *data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) override;

    void doFrameOffset(int &offset) override;

    void setDependentFlag(const QSharedPointer<ProtocolFlagDataSize>& sizeFlag,
                          const QSharedPointer<ProtocolFlagDataContent>& contentFlag,
                          const QSharedPointer<ProtocolFlagDataVerify>& verifyFlag);

    QSharedPointer<ProtocolFlagData> copy() const override;

public:
    QByteArray target;

private:
    QSharedPointer<ProtocolFlagDataSize> mSizeFlag;
    QSharedPointer<ProtocolFlagDataContent> mContentFlag;
    QSharedPointer<ProtocolFlagDataVerify> mVerifyFlag;
};

PROTOCOL_CODEC_NAMESPACE_END