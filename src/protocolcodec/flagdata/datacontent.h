#pragma once

#include <qobject.h>
#include <qmap.h>

#include <protocolcodec/global.h>
#include <protocolcodec/utils/protocolflagdata.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagDataContent : public ProtocolFlagData {
public:
    ProtocolFlagDataContent();
    explicit ProtocolFlagDataContent(int byteSize);

    void setContentSizeFromType(const QMap<int, int>& sizeFromTypeMap);

    bool verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) override;

    int getByteSize() const;

    QByteArray getBytesContent() const override;

public:
    QByteArray contentData;
    bool isFixByteSize;
    QMap<int, int> contentSizeFromType;
};

PROTOCOL_CODEC_NAMESPACE_END