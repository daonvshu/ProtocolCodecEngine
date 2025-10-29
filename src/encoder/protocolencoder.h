#pragma once

#include <qobject.h>
#include <qhash.h>

#include "../protocolcodecinterface.h"
#include "../protocoltypecodec.h"
#include "../utils/protocolflagdata.h"
#include "../protocolmetadata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolEncoder : public ProtocolCodecInterface {
public:
    using ProtocolCodecInterface::ProtocolCodecInterface;
    ~ProtocolEncoder() override;

    void addType(int type, ProtocolTypeCodecInterface* codec);

    ProtocolTypeCodecInterface* getCodec(int type);

    QByteArray encodeFrame(const QByteArray& content, int dataType);

    ProtocolMetaData getLastMetaData() const;

    ProtocolMetaData getLastMetaData(int type) const;

private:
    QHash<int, ProtocolTypeCodecInterface*> typeEncoders;
    QHash<int, ProtocolMetaData> metaData;
    ProtocolMetaData lastMetaData;

private:
    static QByteArray encodeHeader(ProtocolFlagData* flagData);
    static QByteArray encodeAddress(ProtocolFlagData* flagData);
    static QByteArray encodeType(ProtocolFlagData* flagData, int dataType);
    static QByteArray encodeSize(ProtocolFlagData* flagData, int contentSize);
    QByteArray encodeVerify(ProtocolFlagData* flagData, const QByteArray& buffer, int contentSize) const;
    static QByteArray encodeEnd(ProtocolFlagData* flagData);
};

PROTOCOL_CODEC_NAMESPACE_END