#pragma once

#include <qobject.h>
#include <qhash.h>

#include "../protocolcodecinterface.h"
#include "../utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolDecoder : public ProtocolCodecInterface {
public:
    explicit ProtocolDecoder(QObject *parent = nullptr);

    void setFlags(const QList<QSharedPointer<ProtocolFlagData>>& flags) override;

    void addType(int type, const std::function<void(const QByteArray&)>& decoder);

    bool checkRegistered(int type) const;

    void addBuffer(const QByteArray& buffer);

    void setBufferMaxSize(int size);

    void setSizeMaxValue(int value);

    void setDecodeTimeout(int ms);

private:
    QHash<int, std::function<void(const QByteArray&)>> typeDecoders;

    ProtocolFlagData* decodeFlags[256] = { nullptr };
    int flagCount;

    QByteArray bufferCache;

    int mBufferMaxSize;
    int mDecodeTimeout;
    QList<int> validHeaderPos;
    qint64 lastDecodeTimestamp;
};

PROTOCOL_CODEC_NAMESPACE_END