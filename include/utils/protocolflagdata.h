#pragma once

#include "../protocolflags.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagData {
public:
    explicit ProtocolFlagData(ProtocolFlag flag);

    virtual QString dataToString();

public:
    ProtocolFlag flag;
};

inline QDebug operator<<(QDebug debug, ProtocolFlagData* data) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "[";
    debug << data->flag;
    debug << "]";
    auto str = data->dataToString();
    if (!str.isEmpty()) {
        debug << ": ";
        debug << str;
    }
    return debug;
}

class ProtocolFlagDataHeader : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataHeader(QByteArray data);

    QString dataToString() override;

private:
    QByteArray data;
};

class ProtocolFlagDataSize : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataSize(int size);

    QString dataToString() override;

private:
    int size;
};

class ProtocolFlagDataContent : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataContent();

private:

};

class ProtocolFlagDataVerify : public ProtocolFlagData {
public:
    enum VerifyType {
        Crc,
        Sum,
    };

    explicit ProtocolFlagDataVerify(VerifyType verifyType);

    QString dataToString() override;

private:
    VerifyType verifyType;
};

class ProtocolFlagDataEnd : public ProtocolFlagData {
public:
    explicit ProtocolFlagDataEnd(QByteArray data);

    QString dataToString() override;

private:
    QByteArray data;
};

PROTOCOL_CODEC_NAMESPACE_END