#pragma once

#include <qobject.h>
#include <qhash.h>

#include "../protocolcodecinterface.h"
#include "../utils/protocolflagdata.h"
#include "../protocolmetadata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolDecoder : public ProtocolCodecInterface {
public:
    explicit ProtocolDecoder(QObject *parent = nullptr);

    void setFlags(ProtocolFlagData* flags) override;

    void addType(int type, const std::function<void(const QByteArray&)>& decoder);

    bool checkRegistered(int type) const;

    void addBuffer(const QByteArray& buffer);

    void setBufferMaxSize(int size);

    void setSizeMaxValue(int value) const;

    void setDecodeTimeout(int ms);

    void enableMetaDataRecord();

    void clearCache();

    ProtocolMetaData getLastMetaData() const;

    ProtocolMetaData getLastMetaData(int type) const;

private:
    QHash<int, std::function<void(const QByteArray&)>> typeDecoders;
    QHash<int, ProtocolMetaData> metaData;
    ProtocolMetaData lastMetaData;
    bool metadataEnable = false;

    ProtocolFlagData* decodeFlags[256] = { nullptr };
    int flagCount;

    QByteArray bufferCache;

    int mBufferMaxSize;
    int mDecodeTimeout;
    QList<int> validHeaderPos;
    qint64 lastDecodeTimestamp;
};

PROTOCOL_CODEC_NAMESPACE_END