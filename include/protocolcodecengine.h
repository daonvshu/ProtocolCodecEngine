#pragma once

#include <qobject.h>

#include "global.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolCodecEngine : public QObject {
public:
    explicit ProtocolCodecEngine(QObject *parent = nullptr);

    /**
     * @brief 定义协议，例如：H(5AFF)S2CV(CRC)E(FE) （括号表示具体值，数字表示字节数）
     * H：header/帧头
     * S：size/大小
     * C：content/内容（隐式包含数据类型）
     * V：verify/校验类型
     * E：end/帧尾
     * @param templateStr
     */
    void frameDeclare(const QString& templateStr);
};

PROTOCOL_CODEC_NAMESPACE_END