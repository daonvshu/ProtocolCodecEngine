#pragma once

#include <qobject.h>

#include "global.h"
#include "protocoltypecodec.h"

#include <protocolcodec/decoder/protocoldecoder.h>
#include <protocolcodec/encoder/protocolencoder.h>
#include <protocolcodec/verifier/protocolverify.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class PROTOCOL_CODEC_EXPORT ProtocolCodecEngine : public QObject {
public:
    explicit ProtocolCodecEngine(QObject *parent = nullptr);
    ~ProtocolCodecEngine() override;

    /**
     * @brief 定义协议，例如：H(5AFF)T1S2CV(CRC16)E(FE) （括号表示具体值，数字表示字节数）
     * H：header/帧头
     * A：address/地址（可省略）
     * T：type/数据类型（必须显式定义）
     * S：size/大小
     * C：content/内容，不包含独立的 T 字段；C8 表示固定八字节内容
     * V：verify/校验类型
     * E：end/帧尾
     * 除 T 外的字段可按协议省略；变长 C 需要前置 S，或前置 T 配合类型长度表。
     * V 应位于所有参与校验的字段之后；字段缺省与换序限制详见 README。
     * @param templateStr
     */
    void frameDeclare(const QString& templateStr);

    /**
     * @brief 追加参与校验的字段（需要先调用frameDeclare），存在 V 和 C 时默认包含 C；例如 STC 校验长度、类型和内容
     * @param flagStr
     */
    void setVerifyFlags(const QString& flagStr) const;

    /**
     * @brief 追加参与 Size 计算的字段（需要先调用frameDeclare），变长 C 默认参与；例如 TC 表示类型加内容，S 自身不计入
     * @param flagStr
     */
    void setSizeFlags(const QString& flagStr) const;

    /**
     * @brief 设置各类型的内容字节数，用于没有 Size 的变长 C；T 必须在 C 前，且不要把 C 标记为 Size 计算目标
     * @param contentSizeFromType
     */
    void setContentSizeDependsOnType(const QMap<int, int>& contentSizeFromType) const;

    /**
     * @brief 设置地址值
     * @param address
     */
    void setAddressValue(uint32_t address) const;

    /**
     * @brief 自定义校验器
     * @param customVerifier
     */
    void setCustomVerifier(const QSharedPointer<ProtocolVerify>& customVerifier) const;

    /**
     * @brief 设置帧长度字节最大值
     * @param value
     */
    void setSizeMaxValue(int value) const;

    /**
     * @brief 设置帧缓存最大值
     * @param size
     */
    void setBufferMaxSize(int size);

    /**
     * @brief 设置断帧解析超时时间，超时后将清理缓存
     * @param ms
     */
    void setDecodeTimeout(int ms);

    /**
     * @brief 设置日志筛选器
     * @param categoryPtr
     */
    void setLogging(LoggingCategoryPtr categoryPtr);

    /**
     * @brief 设置解析错误日志标志，使用 setLogging 指定的分类输出完整错误帧字节
     * @param flags 按位组合 DecodeError；None 关闭，All 全开；不影响编码日志
     */
    void setDecodeErrorLogFlags(DecodeErrorFlags flags);

    /**
     * @brief 注册类型和解析回调
     * @tparam T 类型
     * @tparam K 回调类类型
     * @param context
     * @param callback
     */
    template<typename Codec, typename T, typename K>
    void registerType(K* context, void(K::*callback)(const T& t)) {
        decoder.addType(T::Type, [context, callback] (const QByteArray& content) {
            (context->*callback)(Codec().decode(content));
        });
        Q_ASSERT_X(encoder.getCodec(T::Type) == nullptr, "ProtocolCodecEngine::registerType", "Type already registered!");
        encoder.addType(T::Type, new Codec);
    }

    /**
     * @brief 注册类型，不使用其解码器，只编码
     * @tparam T
     * @tparam Codec
     */
    template<typename T, template<typename> class Codec>
    void registerType() {
        Q_ASSERT_X(encoder.getCodec(T::Type) == nullptr, "ProtocolCodecEngine::registerType", "Type already registered!");
        encoder.addType(T::Type, new Codec<T>);
    }

    /**
     * @brief 注册压缩编码器，只编码
     * @tparam T
     * @tparam Codec
     */
    template<typename T, template<typename> class Codec, int Level = -1>
    void registerCompressEncoder() {
        Q_ASSERT_X(encoder.getCodec(T::Type) == nullptr, "ProtocolCodecEngine::registerType", "Type already registered!");
        encoder.addType(T::Type, new CompressCodec<T, Codec, Level>);
    }

    /**
     * @brief 注册类型，仅定义命令
     * @tparam I
     * @tparam T
     * @param context
     * @param callback
     */
    template<int I, typename T>
    void registerType(T* context, void(T::*callback)()) {
        Q_ASSERT_X(!decoder.checkRegistered(I), "ProtocolCodecEngine::registerType", "Type already registered!");
        decoder.addType(I, [context, callback] (const QByteArray& content) {
            (context->*callback)();
        });
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
     * @brief 数据编码，仅编码
     * @tparam I
     * @return
     */
    template<int I>
    QByteArray encode() {
        return encoder.encodeFrame(QByteArray(), I);
    }

    /**
     * @brief 添加新的待解析缓存
     * @param buffer
     */
    void appendBuffer(const QByteArray& buffer);

    /**
     * @brief 清空缓存
     */
    void clearDecodeCacheBuffer();

    /**
     * @brief 开启记录解码元数据
     */
    void enableRecordDecodeMetaData();

    /**
     * @brief 获取最后一次编码的元数据
     * @return
     */
    ProtocolMetaData getLastEncodeMetaData() const;

    /**
     * @brief 获取指定类型最后一次编码的元数据
     * @param type
     * @return
     */
    ProtocolMetaData getLastEncodeMetaData(int type) const;

    /**
     * @brief 获取最后一次解码的元数据
     * @return
     */
    ProtocolMetaData getLastDecodeMetaData() const;

    /**
     * @brief 获取指定类型最后一次解码的元数据
     * @param type
     * @return
     */
    ProtocolMetaData getLastDecodeMetaData(int type) const;

private:
    static void clearFlags(ProtocolFlagData* flags);
    QList<ProtocolFlagData*> getFlagDataByFlags(ProtocolFlagData* makeFlags) const;

private:
    ProtocolFlagData* flags = nullptr;
    ProtocolDecoder decoder;
    ProtocolEncoder encoder;
};

PROTOCOL_CODEC_NAMESPACE_END
