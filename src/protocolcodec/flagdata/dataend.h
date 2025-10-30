#pragma once

#include <qobject.h>

#include <protocolcodec/global.h>
#include <protocolcodec/utils/protocolflagdata.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataEnd : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataEnd(QByteArray data);

    QString dataToString() override;

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) override;

    QByteArray getBytesContent() const override;

public:
    QByteArray target;
};

PROTOCOL_CODEC_NAMESPACE_END