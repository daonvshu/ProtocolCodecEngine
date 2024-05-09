#pragma once

#include <qbytearray.h>
#include <qdatastream.h>

#include "global.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolTypeCodecInterface {
public:
    virtual ~ProtocolTypeCodecInterface() = default;
};

template<typename T>
class ProtocolTypeCodec : public ProtocolTypeCodecInterface {
public:
    virtual T decode(const QByteArray& content) = 0;
    virtual QByteArray encode(const T& data) = 0;
};

template<typename T>
class JsonCodec : public ProtocolTypeCodec<T> {
public:
    T decode(const QByteArray &content) override {
        return T::fromJson(content);
    }

    QByteArray encode(const T &data) override {
        return data.toJson();
    }
};

template<typename T>
class SerializeCodec : public ProtocolTypeCodec<T> {
public:
    T decode(const QByteArray &content) override {
        QDataStream in(content);
        T data;
        in >> data;
        return data;
    }

    QByteArray encode(const T &data) override {
        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << data;
        return buffer;
    }
};

PROTOCOL_CODEC_NAMESPACE_END