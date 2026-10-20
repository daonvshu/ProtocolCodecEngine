#pragma once

#include <qobject.h>
#include <qhash.h>

#include <protocolcodec/protocolcodecinterface.h>
#include <protocolcodec/utils/protocolflagdata.h>
#include <protocolcodec/protocolmetadata.h>

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

    void setDecodeErrorLogFlags(DecodeErrorFlags flags);

    void enableMetaDataRecord();

    void clearCache();

    ProtocolMetaData getLastMetaData() const;

    ProtocolMetaData getLastMetaData(int type) const;

private:
    friend class MultiProtocolCodec;
    enum class DecodeResult { Complete, Incomplete, Invalid };

    DecodeResult decodeFrame(QByteArray& buffer, int offset, ProtocolMetaData* metadata, int& frameSize,
                             int& type, QByteArray& content, DecodeError& error, QString& detail);
    bool errorLoggingEnabled(DecodeError error) const;
    void logDecodeError(DecodeError error, const QString& detail, const QByteArray& bytes,
                        bool completeFrame) const;

    QHash<int, std::function<void(const QByteArray&)>> typeDecoders;
    QHash<int, ProtocolMetaData> metaData;
    ProtocolMetaData lastMetaData;
    bool metadataEnable = false;

    ProtocolFlagData* decodeFlags[256] = { nullptr };
    int flagCount;

    QByteArray bufferCache;

    int mBufferMaxSize;
    int mDecodeTimeout;
    qint64 lastDecodeTimestamp;
    DecodeErrorFlags errorLogFlags = DecodeError::AddressMismatch | DecodeError::InvalidLength
        | DecodeError::ChecksumMismatch | DecodeError::UnknownType | DecodeError::Timeout;
};

PROTOCOL_CODEC_NAMESPACE_END
