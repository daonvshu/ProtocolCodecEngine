#pragma once

#include <qobject.h>

#include "global.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataSize : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataSize(int byteSize);

    QString dataToString() override;

    bool verify(char *data, int offset, int maxSize) override;

    void doFrameOffset(int &offset) override;

    QSharedPointer<ProtocolFlagData> copy() const override;

    void setSizeMaxValue(int value);

public:
    int byteSize;
    int dataSize;
    int sizeValueMax;
};

PROTOCOL_CODEC_NAMESPACE_END