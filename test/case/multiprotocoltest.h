#pragma once

#include <QObject>
#include <QMap>
#include <functional>
#include <protocolcodec/multiprotocolcodec.h>

struct MultiProtocolPacket {
    enum { Type = 0x21 };
    QByteArray content;
    static MultiProtocolPacket fromBytes(const QByteArray& bytes) { return {bytes}; }
    QByteArray toBytes() const { return content; }
};

class MultiProtocolTest : public QObject {
    Q_OBJECT

private slots:
    void configurationValidation();
    void independentEncoding();
    void encodeOnlyProtocols();
    void rawContentCallbacks();
    void multiProtocolRawCallbacks();
    void nestedContentRouting();
    void mixedPartitions();
    void nestedFrameWait();
    void badFramesRecover();
    void sizeLessAndCommands();
    void layoutVariants();
    void loggingFlags();
    void timeoutAndCacheLimit();
    void reentrantAndClear();
    void callbackException();
    void metadataIsolation();
    void hundredMiBStream();
    void cleanup();

private:
    void configure(protocol_codec::MultiProtocolCodec& codec);
    void receiveOne(const MultiProtocolPacket& packet);
    void receiveTwo(const MultiProtocolPacket& packet);
    void receiveThree(const MultiProtocolPacket& packet);
    void receiveFour(const MultiProtocolPacket& packet);
    void receive(int protocol, const MultiProtocolPacket& packet);
    void commandOne();
    void commandTwo();

    QList<int> receivedProtocols;
    QList<QByteArray> receivedContents;
    QMap<int, quint64> receivedCounts;
    QMap<int, QByteArray> expectedContents;
    std::function<void(int)> onReceive;
    bool recordEvents = true;
    bool payloadsMatch = true;
};
