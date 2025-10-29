#pragma once

#include <qobject.h>

#include "../global.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataSize : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataSize(int byteSize);

    QString dataToString() override;

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) override;

    void setSizeMaxValue(int value);

    int getContentSize() const;

    QByteArray getBytesContent() const override;

public:
    int dataSize;
    int sizeValueMax;
};

PROTOCOL_CODEC_NAMESPACE_END