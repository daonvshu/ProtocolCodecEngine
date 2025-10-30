#pragma once

#include <protocolcodec/utils/protocolflagdata.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataType : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataType(int byteSize);

    QString dataToString() override;

    bool verify(char* data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory&(* debugPtr)()) override;

    QByteArray getBytesContent() const override;

public:
    int value;
};

PROTOCOL_CODEC_NAMESPACE_END