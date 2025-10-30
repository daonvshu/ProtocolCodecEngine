#pragma once

#include <protocolcodec/utils/protocolflagdata.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataAddress : public ProtocolFlagData
{
public:
    explicit ProtocolFlagDataAddress(int byteSize);

    QString dataToString() override;

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory &(*debugPtr)()) override;

    QByteArray getBytesContent() const override;

public:
    uint32_t address;
};


PROTOCOL_CODEC_NAMESPACE_END