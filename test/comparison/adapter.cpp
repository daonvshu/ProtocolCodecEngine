#include "adapter.h"
#include <protocolcodec/protocolcodecengine.h>
#include <utility>

#ifdef CODEC_BASELINE
namespace codec = protocol_codec_baseline;
#define ADAPTER_FACTORY makeBaseline
#else
namespace codec = protocol_codec;
#define ADAPTER_FACTORY makeCurrent
#endif

namespace {
Q_LOGGING_CATEGORY(comparisonLog, "protocol.comparison")

template<int Value> struct Packet {
    enum { Type = Value };
    QByteArray content;
    static Packet fromBytes(const QByteArray& content) { return {content}; }
    QByteArray toBytes() const { return content; }
};

class Adapter : public DecoderAdapter {
public:
    Adapter(const DecoderConfig& config, PacketCallback callback)
        : callback(std::move(callback)), recordMetadata(config.metadata) {
        engine.frameDeclare(config.format);
        engine.setVerifyFlags(config.verifyFlags);
        if (!config.typeLength) engine.setSizeFlags(config.sizeFlags);
        engine.setAddressValue(10);
        engine.setSizeMaxValue(config.maximumSize);
        engine.setBufferMaxSize(config.bufferLimit);
        engine.setDecodeTimeout(config.timeout);
        if (config.metadata) engine.enableRecordDecodeMetaData();
        if (config.typeLength) engine.setContentSizeDependsOnType({{100, 8}, {101, 8}});
        engine.registerType<codec::BytesCodec<Packet<100>>>(this, &Adapter::receive100);
        engine.registerType<codec::BytesCodec<Packet<101>>>(this, &Adapter::receive101);
#ifndef CODEC_BASELINE
        engine.setDecodeErrorLogFlags(config.logging ? codec::DecodeError::All : codec::DecodeError::None);
#endif
        if (config.logging) engine.setLogging(comparisonLog);
    }

    void feed(const QByteArray& bytes) override { engine.appendBuffer(bytes); }
    QByteArray encode(const QByteArray& bytes, int type) override {
        return type == 100 ? engine.encode(Packet<100>{bytes}) : engine.encode(Packet<101>{bytes});
    }
    void clear() override { engine.clearDecodeCacheBuffer(); }
    QByteArray successfulMetadata() const override {
        const auto metadata = engine.getLastDecodeMetaData();
        return metadata.header + metadata.type + metadata.address + metadata.size
            + metadata.content + metadata.verify + metadata.end + metadata.localVerifyCode;
    }

private:
    void receive100(const Packet<100>& packet) {
        callback({100, packet.content, recordMetadata ? successfulMetadata() : QByteArray()});
    }
    void receive101(const Packet<101>& packet) {
        callback({101, packet.content, recordMetadata ? successfulMetadata() : QByteArray()});
    }
    codec::ProtocolCodecEngine engine;
    PacketCallback callback;
    bool recordMetadata;
};
}

std::unique_ptr<DecoderAdapter> ADAPTER_FACTORY(const DecoderConfig& config, PacketCallback callback) {
    return std::unique_ptr<DecoderAdapter>(new Adapter(config, std::move(callback)));
}
