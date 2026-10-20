#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <functional>
#include <memory>

struct DecoderConfig {
    QString format = "H(5AFA)T1A1S2CV(CRC16)E(FEEF)";
    QString verifyFlags = "STAC";
    QString sizeFlags = "TC";
    int maximumSize = 60000;
    int bufferLimit = 65536;
    int timeout = -1;
    bool metadata = false;
    bool logging = false;
    bool typeLength = false;
};

struct DecodedPacket {
    int type;
    QByteArray content;
    QByteArray metadata;
    bool operator==(const DecodedPacket& other) const {
        return type == other.type && content == other.content && metadata == other.metadata;
    }
};

class DecoderAdapter {
public:
    virtual ~DecoderAdapter() = default;
    virtual void feed(const QByteArray& bytes) = 0;
    virtual QByteArray encode(const QByteArray& payload, int type = 100) = 0;
    virtual void clear() = 0;
    virtual QByteArray successfulMetadata() const = 0;
};

using PacketCallback = std::function<void(const DecodedPacket&)>;
std::unique_ptr<DecoderAdapter> makeBaseline(const DecoderConfig& config, PacketCallback callback);
std::unique_ptr<DecoderAdapter> makeCurrent(const DecoderConfig& config, PacketCallback callback);
