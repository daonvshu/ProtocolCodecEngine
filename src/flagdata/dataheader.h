#pragma once

#include <qobject.h>

#include "../global.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataHeader : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataHeader(QByteArray data);

    QString dataToString() override;

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) override;

    QByteArray getBytesContent() const override;

public:
    QByteArray target;
};

PROTOCOL_CODEC_NAMESPACE_END