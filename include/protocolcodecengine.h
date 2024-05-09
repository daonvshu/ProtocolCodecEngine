#pragma once

#include <qobject.h>

#include "global.h"
#include "protocoltypecodec.h"

#include "decoder/protocoldecoder.h"
#include "encoder/protocolencoder.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolCodecEngine : public QObject {
public:
    explicit ProtocolCodecEngine(QObject *parent = nullptr);

    /**
     * @brief 定义协议，例如：H(5AFF)S2CV(CRC16)E(FE) （括号表示具体值，数字表示字节数）
     * H：header/帧头
     * S：size/大小
     * C：content/内容（隐式包含数据类型）
     * V：verify/校验类型
     * E：end/帧尾
     * @param templateStr
     */
    void frameDeclare(const QString& templateStr);

    /**
     * @brief 设置校验Flag（需要先调用frameDeclare），例如：HSC，表示校验帧头、大小、内容所有字节数
     * @param flagStr
     */
    void setVerifyFlags(const QString& flagStr);

    /**
     * @brief 注册类型和解析回调
     * @tparam T 类型
     * @tparam K 回调类类型
     * @param context
     * @param callback
     * @param codec 编码器
     */
    template<typename Codec, typename T, typename K>
    void registerType(K* context, void(K::*callback)(const T& t)) {
        decoder.addType(T::Type, [context, callback] (const QByteArray& content) {
            (context->*callback)(Codec().decode(content));
        });
        encoder.addType(T::Type, new Codec);
    }

    /**
     * @brief 数据编码
     * @tparam T
     * @param data
     * @return
     */
    template<typename T>
    QByteArray encode(const T& data) {
        auto codec = encoder.getCodec(T::Type);
        auto content = dynamic_cast<ProtocolTypeCodec<T>*>(codec)->encode(data);
        return encoder.encodeFrame(content, T::Type);
    }

    /**
     * @brief 添加新的待解析缓存
     * @param buffer
     */
    void appendBuffer(const QByteArray& buffer);

private:
    ProtocolDecoder decoder;
    ProtocolEncoder encoder;
};

PROTOCOL_CODEC_NAMESPACE_END