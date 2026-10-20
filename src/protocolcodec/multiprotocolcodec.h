#pragma once

#include "protocolcodecengine.h"

#include <QMap>
#include <QList>
#include <memory>

PROTOCOL_CODEC_NAMESPACE_BEGIN

class PROTOCOL_CODEC_EXPORT MultiProtocolCodec : public QObject {
public:
    struct ProtocolDefinition {
        QString frameTemplate;
        QString verifyFlags;
        QString sizeFlags;
        QMap<int, int> contentSizeFromType;
        quint32 address = 0;
        int maximumSize = -1;
        bool encodeOnly = false; // true 时仅用于编码，不参与接收解析
    };

    explicit MultiProtocolCodec(QObject* parent = nullptr);
    ~MultiProtocolCodec() override;

    /**
     * @brief 注册结构相同、帧头/帧尾/地址特征不同的协议；无效或重复配置抛出 ProtocolException
     * @param protocolId 非负且唯一的协议编号
     * @param definition 字段定义与公共解析参数；帧头必须是首字段
     */
    void addProtocol(int protocolId, const ProtocolDefinition& definition);

    /**
     * @brief 使用默认参数注册协议；默认只将 C 纳入长度及校验范围
     */
    void addProtocol(int protocolId, const QString& frameTemplate);

    /**
     * @brief 按注册顺序返回协议编号
     */
    QList<int> protocolIds() const;

    /**
     * @brief 清除协议及接收缓存；不能在解析回调中调用
     */
    void clearProtocols();

    /**
     * @brief 为指定协议注册内容编解码器及回调，不同协议允许使用相同的 Type
     */
    template<typename Codec, typename T, typename K>
    void registerType(int protocolId, K* context, void(K::*callback)(const T&)) {
        requireProtocol(protocolId).registerType<Codec>(context, callback);
    }

    /**
     * @brief 为指定协议注册仅编码的内容编解码器
     */
    template<typename T, template<typename> class Codec>
    void registerType(int protocolId) {
        requireProtocol(protocolId).registerType<T, Codec>();
    }

    /**
     * @brief 为指定协议注册无内容命令回调
     */
    template<int Type, typename K>
    void registerType(int protocolId, K* context, void(K::*callback)()) {
        requireProtocol(protocolId).registerType<Type>(context, callback);
    }

    /**
     * @brief 使用指定协议编码内容
     */
    template<typename T>
    QByteArray encode(int protocolId, const T& data) {
        return requireProtocol(protocolId).encode(data);
    }

    /**
     * @brief 使用指定协议编码无内容命令
     */
    template<int Type>
    QByteArray encode(int protocolId) {
        return requireProtocol(protocolId).encode<Type>();
    }

    /**
     * @brief 添加接收字节，按线上顺序分发；不完整候选帧等待收齐，避免抢占其内部的帧头
     */
    void appendBuffer(const QByteArray& buffer);

    /**
     * @brief 清除公共接收缓存和排队输入，可在回调中调用
     */
    void clearDecodeCacheBuffer();

    /**
     * @brief 设置公共缓存及可接收整帧的大小上限，非正数表示不限制
     */
    void setBufferMaxSize(int size);

    /**
     * @brief 设置接收间隔超时，下一次收到数据时清除过期候选帧，非正数关闭
     */
    void setDecodeTimeout(int milliseconds);

    /**
     * @brief 设置所有协议及公共接收诊断的日志分类
     */
    void setLogging(LoggingCategoryPtr category);

    /**
     * @brief 设置所有协议及公共接收诊断的错误日志标志
     */
    void setDecodeErrorLogFlags(DecodeErrorFlags flags);

    /**
     * @brief 开启所有协议的成功帧元数据记录
     */
    void enableRecordDecodeMetaData();

    /**
     * @brief 返回最近接受的协议编号，尚未接受任何帧时返回 -1
     */
    int lastDecodedProtocolId() const;

    /**
     * @brief 获取指定协议最后成功解码的元数据
     */
    ProtocolMetaData getLastDecodeMetaData(int protocolId) const;

    /**
     * @brief 获取指定协议、指定类型最后成功解码的元数据
     */
    ProtocolMetaData getLastDecodeMetaData(int protocolId, int type) const;

    /**
     * @brief 获取指定协议最后编码的元数据
     */
    ProtocolMetaData getLastEncodeMetaData(int protocolId) const;

private:
    ProtocolCodecEngine& requireProtocol(int protocolId) const;
    void processBuffer();
    struct Private;
    std::unique_ptr<Private> d;
};

PROTOCOL_CODEC_NAMESPACE_END
