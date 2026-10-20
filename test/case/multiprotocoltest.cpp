#include "multiprotocoltest.h"

#include <QtTest>
#include <QElapsedTimer>
#include <protocolcodec/protocolexception.h>
#include <climits>
#include <stdexcept>

using namespace protocol_codec;

namespace {
Q_LOGGING_CATEGORY(multiProtocolTestLog, "protocol.multi.test")
QStringList* capturedLogs = nullptr;

void captureMessage(QtMsgType type, const QMessageLogContext&, const QString& message) {
    if (capturedLogs != nullptr && type == QtWarningMsg) capturedLogs->append(message);
}

struct LogCapture {
    QStringList messages;
    QtMessageHandler previous;
    LogCapture() : previous(qInstallMessageHandler(captureMessage)) { capturedLogs = &messages; }
    ~LogCapture() { capturedLogs = nullptr; qInstallMessageHandler(previous); }
};

MultiProtocolCodec::ProtocolDefinition definition(const QString& header = "AA55",
                                                 const QString& tail = "0D0A", quint32 address = 1) {
    MultiProtocolCodec::ProtocolDefinition result;
    result.frameTemplate = "H(" + header + ")T1A1S2CV(CRC16)E(" + tail + ")";
    result.verifyFlags = "HTASC";
    result.sizeFlags = "TC";
    result.address = address;
    result.maximumSize = 512;
    return result;
}

QByteArray independentFrame(const QByteArray& content, const QByteArray& header = QByteArray::fromHex("aa55"),
                            const QByteArray& tail = QByteArray::fromHex("0d0a"), int address = 1,
                            int type = MultiProtocolPacket::Type) {
    QByteArray result = header;
    result.append(char(type));
    result.append(char(address));
    const int size = content.size() + 1;
    result.append(char(size));
    result.append(char(size >> 8));
    result += content;
    quint16 crc = 0xffff;
    for (unsigned char byte : result) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
    }
    result.append(char(crc));
    result.append(char(crc >> 8));
    return result + tail;
}
}

void MultiProtocolTest::configure(MultiProtocolCodec& codec) {
    codec.setDecodeErrorLogFlags(DecodeError::None);
    codec.addProtocol(1, definition());
    codec.addProtocol(2, definition("AA66", "0E0B", 2));
    codec.addProtocol(3, definition("AA55", "0E0B", 1));
    codec.addProtocol(4, definition("AA55", "0D0A", 3));
    codec.registerType<BytesCodec<MultiProtocolPacket>>(1, this, &MultiProtocolTest::receiveOne);
    codec.registerType<BytesCodec<MultiProtocolPacket>>(2, this, &MultiProtocolTest::receiveTwo);
    codec.registerType<BytesCodec<MultiProtocolPacket>>(3, this, &MultiProtocolTest::receiveThree);
    codec.registerType<BytesCodec<MultiProtocolPacket>>(4, this, &MultiProtocolTest::receiveFour);
}

void MultiProtocolTest::configurationValidation() {
    MultiProtocolCodec codec;
    QCOMPARE(codec.lastDecodedProtocolId(), -1);
    codec.addProtocol(1, definition());
    QCOMPARE(codec.protocolIds(), QList<int>{1});
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(1, definition("AA66")), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(-1, definition()), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, definition()), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "T1S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)T1T1S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)T8S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA5)T1S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)T1C9999999999999999999"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)T1C8S2"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)T1CS2"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA55)T1V(CRC16)S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA66)T1S2C"), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, "H(AA66)T1C"), ProtocolException);
    auto differentHeaderLength = definition("AA55AA");
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, differentHeaderLength), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.encode<7>(99), ProtocolException);
    auto invalid = definition("AA66");
    invalid.verifyFlags = "HTASCV";
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, invalid), ProtocolException);
    invalid.verifyFlags = "HTASC";
    invalid.address = 256;
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, invalid), ProtocolException);
    QCOMPARE(codec.protocolIds(), QList<int>{1});
    codec.appendBuffer(QByteArray::fromHex("aa"));
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(2, definition("AA66")), ProtocolException);
    codec.clearDecodeCacheBuffer();
    codec.addProtocol(2, definition("AA66"));
    QCOMPARE(codec.protocolIds(), (QList<int>{1, 2}));
    codec.clearProtocols();
    QVERIFY(codec.protocolIds().isEmpty());
    codec.addProtocol(9, "H(1122)T1E(3344)");
    QCOMPARE(codec.encode<7>(9), QByteArray::fromHex("1122073344"));
}

void MultiProtocolTest::independentEncoding() {
    MultiProtocolCodec codec;
    configure(codec);
    const QByteArray content = QByteArray::fromHex("010203040506");
    QCOMPARE(codec.encode(1, MultiProtocolPacket{content}), independentFrame(content));
    QCOMPARE(codec.encode(2, MultiProtocolPacket{content}), independentFrame(content,
        QByteArray::fromHex("aa66"), QByteArray::fromHex("0e0b"), 2));
    QCOMPARE(codec.encode(3, MultiProtocolPacket{content}), independentFrame(content,
        QByteArray::fromHex("aa55"), QByteArray::fromHex("0e0b"), 1));
    QCOMPARE(codec.encode(4, MultiProtocolPacket{content}), independentFrame(content,
        QByteArray::fromHex("aa55"), QByteArray::fromHex("0d0a"), 3));
    QCOMPARE(codec.getLastEncodeMetaData(2).header, QByteArray::fromHex("aa66"));
}

void MultiProtocolTest::encodeOnlyProtocols() {
    MultiProtocolCodec codec;
    codec.enableRecordDecodeMetaData();
    codec.setDecodeErrorLogFlags(DecodeError::None);
    auto sendOnly = definition();
    QVERIFY(!sendOnly.encodeOnly);
    sendOnly.encodeOnly = true;
    codec.addProtocol(1, sendOnly);
    codec.registerType<BytesCodec<MultiProtocolPacket>>(1, this, &MultiProtocolTest::receiveOne);

    const QByteArray content = QByteArray::fromHex("1234");
    const auto ignored = independentFrame(content);
    QCOMPARE(codec.encode(1, MultiProtocolPacket{content}), ignored);
    codec.appendBuffer(ignored + QByteArray::fromHex("aa55"));
    QVERIFY(receivedProtocols.isEmpty());
    QCOMPARE(codec.lastDecodedProtocolId(), -1);

    // An encode-only protocol must not retain even a partial header in the receive cache.
    codec.addProtocol(2, definition("AA66", "0E0B", 2));
    codec.registerType<BytesCodec<MultiProtocolPacket>>(2, this, &MultiProtocolTest::receiveTwo);
    const auto accepted = independentFrame(content, QByteArray::fromHex("aa66"), QByteArray::fromHex("0e0b"), 2);
    // An incomplete encode-only frame must not block the following receive protocol.
    codec.appendBuffer(ignored.left(6) + accepted);
    QCOMPARE(receivedProtocols, QList<int>{2});
    QCOMPARE(codec.lastDecodedProtocolId(), 2);
    QCOMPARE(codec.getLastDecodeMetaData(2).content, content);
    QCOMPARE(codec.getLastDecodeMetaData(1).decodeType, INT_MAX);

    // Identical features are allowed when only one of the protocols participates in decoding.
    codec.addProtocol(3, definition());
    codec.registerType<BytesCodec<MultiProtocolPacket>>(3, this, &MultiProtocolTest::receiveThree);
    codec.appendBuffer(ignored + accepted);
    QCOMPARE(receivedProtocols, (QList<int>{2, 3, 2}));
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(4, definition()), ProtocolException);
    codec.addProtocol(4, sendOnly);
    codec.registerType<MultiProtocolPacket, BytesCodec>(4);
    QCOMPARE(codec.encode(4, MultiProtocolPacket{content}), ignored);
    QCOMPARE(codec.protocolIds(), (QList<int>{1, 2, 3, 4}));

    codec.clearProtocols();
    codec.addProtocol(1, sendOnly);
    codec.setLogging(multiProtocolTestLog);
    codec.setDecodeErrorLogFlags(DecodeError::All);
    {
        LogCapture logs;
        codec.appendBuffer(ignored + QByteArray::fromHex("aa55"));
        QVERIFY(logs.messages.isEmpty());
    }
    QVERIFY_EXCEPTION_THROWN(codec.encode<7>(2), ProtocolException);
    QCOMPARE(codec.lastDecodedProtocolId(), -1);
    codec.addProtocol(2, definition("AA66", "0E0B", 2));
    codec.registerType<BytesCodec<MultiProtocolPacket>>(2, this, &MultiProtocolTest::receiveTwo);
    codec.appendBuffer(accepted);
    QCOMPARE(receivedProtocols, (QList<int>{2, 3, 2, 2}));
}

void MultiProtocolTest::rawContentCallbacks() {
    ProtocolCodecEngine codec;
    const auto config = definition();
    codec.frameDeclare(config.frameTemplate);
    codec.setVerifyFlags(config.verifyFlags);
    codec.setSizeFlags(config.sizeFlags);
    codec.setAddressValue(config.address);
    codec.setDecodeErrorLogFlags(DecodeError::None);
    codec.enableRecordDecodeMetaData();
    QList<QByteArray> contents;
    codec.registerRawType(MultiProtocolPacket::Type, [&](const QByteArray& content) { contents.append(content); });
    const QByteArray content = QByteArray::fromHex("0001aa550d0aff00");
    const auto frame = independentFrame(content);
    for (int split = 0; split <= frame.size(); ++split) {
        contents.clear();
        codec.clearDecodeCacheBuffer();
        codec.appendBuffer(frame.left(split));
        QCOMPARE(contents.size(), split == frame.size() ? 1 : 0);
        codec.appendBuffer(frame.mid(split));
        QCOMPARE(contents, QList<QByteArray>{content});
        QCOMPARE(codec.getLastDecodeMetaData().content, content);
    }
    contents.clear();
    codec.appendBuffer(independentFrame(QByteArray()));
    QCOMPARE(contents, QList<QByteArray>{QByteArray()});

    codec.registerType<BytesCodec<MultiProtocolPacket>>(this, &MultiProtocolTest::receiveOne);
    codec.appendBuffer(frame);
    QCOMPARE(receivedProtocols, QList<int>{1});
    contents.clear();
    codec.registerRawType(MultiProtocolPacket::Type, [&](const QByteArray& bytes) { contents.append(bytes); });
    QVERIFY_EXCEPTION_THROWN(codec.registerRawType(MultiProtocolPacket::Type, {}), ProtocolException);
    codec.appendBuffer(frame);
    QCOMPARE(contents, QList<QByteArray>{content});
    QCOMPARE(receivedProtocols, QList<int>{1});
    QCOMPARE(codec.encode(MultiProtocolPacket{content}), frame);

    codec.frameDeclare("H(1122)T1E(3344)");
    codec.registerRawType(7, [&](const QByteArray& bytes) { contents.append(bytes); });
    codec.appendBuffer(QByteArray::fromHex("1122073344"));
    QCOMPARE(contents, (QList<QByteArray>{content, QByteArray()}));
}

void MultiProtocolTest::multiProtocolRawCallbacks() {
    MultiProtocolCodec codec;
    configure(codec);
    QList<int> protocols;
    QList<QByteArray> contents;
    codec.registerRawType(1, MultiProtocolPacket::Type, [&](const QByteArray& bytes) {
        protocols.append(1);
        contents.append(bytes);
    });
    codec.registerRawType(2, MultiProtocolPacket::Type, [&](const QByteArray& bytes) {
        protocols.append(2);
        contents.append(bytes);
    });
    QVERIFY_EXCEPTION_THROWN(codec.registerRawType(99, MultiProtocolPacket::Type,
        [](const QByteArray&) {}), ProtocolException);
    QVERIFY_EXCEPTION_THROWN(codec.registerRawType(1, MultiProtocolPacket::Type, {}), ProtocolException);
    const QByteArray first = QByteArray::fromHex("0001aa00");
    const QByteArray second = QByteArray::fromHex("ff0055ff");
    const auto firstFrame = independentFrame(first);
    const auto secondFrame = independentFrame(second, QByteArray::fromHex("aa66"), QByteArray::fromHex("0e0b"), 2);
    const auto thirdFrame = independentFrame(first, QByteArray::fromHex("aa55"), QByteArray::fromHex("0e0b"), 1);
    codec.appendBuffer(firstFrame + secondFrame + thirdFrame);
    QCOMPARE(protocols, (QList<int>{1, 2}));
    QCOMPARE(contents, (QList<QByteArray>{first, second}));
    QCOMPARE(receivedProtocols, QList<int>{3});
    QCOMPARE(codec.encode(1, MultiProtocolPacket{first}), firstFrame);

    auto sendOnly = definition("AA77");
    sendOnly.encodeOnly = true;
    codec.addProtocol(5, sendOnly);
    codec.registerRawType(5, MultiProtocolPacket::Type, [&](const QByteArray&) { protocols.append(5); });
    codec.appendBuffer(independentFrame(first, QByteArray::fromHex("aa77")) + firstFrame);
    QCOMPARE(protocols, (QList<int>{1, 2, 1}));
}

void MultiProtocolTest::nestedContentRouting() {
    ProtocolCodecEngine outer;
    const auto config = definition("1122", "3344");
    outer.frameDeclare(config.frameTemplate);
    outer.setVerifyFlags(config.verifyFlags);
    outer.setSizeFlags(config.sizeFlags);
    outer.setAddressValue(config.address);
    outer.setDecodeErrorLogFlags(DecodeError::None);
    MultiProtocolCodec inner;
    configure(inner);
    QList<QByteArray> outerContents;
    outer.registerRawType(MultiProtocolPacket::Type, [&](const QByteArray& bytes) {
        outerContents.append(bytes);
        inner.appendBuffer(bytes);
    });
    const QByteArray content = QByteArray::fromHex("0001ffaa00");
    const auto innerFrame = independentFrame(content);
    const auto secondInner = independentFrame(content, QByteArray::fromHex("aa66"), QByteArray::fromHex("0e0b"), 2);
    const auto wrap = [&](const QByteArray& bytes) {
        return independentFrame(bytes, QByteArray::fromHex("1122"), QByteArray::fromHex("3344"));
    };
    const auto frame = wrap(innerFrame);
    for (int split = 0; split <= frame.size(); ++split) {
        outerContents.clear();
        receivedProtocols.clear();
        receivedContents.clear();
        outer.clearDecodeCacheBuffer();
        inner.clearDecodeCacheBuffer();
        outer.appendBuffer(frame.left(split));
        QCOMPARE(receivedProtocols.size(), split == frame.size() ? 1 : 0);
        outer.appendBuffer(frame.mid(split));
        QCOMPARE(outerContents, QList<QByteArray>{innerFrame});
        QCOMPARE(receivedProtocols, QList<int>{1});
        QCOMPARE(receivedContents, QList<QByteArray>{content});
    }

    auto corruptOuter = frame;
    corruptOuter[corruptOuter.size() - 4] = char(uchar(corruptOuter.at(corruptOuter.size() - 4)) ^ 1);
    outerContents.clear();
    receivedProtocols.clear();
    outer.appendBuffer(corruptOuter + frame);
    QCOMPARE(outerContents, QList<QByteArray>{innerFrame});
    QCOMPARE(receivedProtocols, QList<int>{1});

    auto corruptInner = innerFrame;
    corruptInner[corruptInner.size() - 4] = char(uchar(corruptInner.at(corruptInner.size() - 4)) ^ 1);
    outerContents.clear();
    receivedProtocols.clear();
    outer.appendBuffer(wrap(corruptInner + innerFrame + secondInner));
    QCOMPARE(outerContents, QList<QByteArray>{corruptInner + innerFrame + secondInner});
    QCOMPARE(receivedProtocols, (QList<int>{1, 2}));

    outerContents.clear();
    receivedProtocols.clear();
    receivedContents.clear();
    outer.appendBuffer(wrap(innerFrame.left(5)));
    QVERIFY(receivedProtocols.isEmpty());
    outer.appendBuffer(wrap(innerFrame.mid(5)));
    QCOMPARE(outerContents, (QList<QByteArray>{innerFrame.left(5), innerFrame.mid(5)}));
    QCOMPARE(receivedProtocols, QList<int>{1});
    QCOMPARE(receivedContents, QList<QByteArray>{content});

    ProtocolCodecEngine differentInner;
    differentInner.frameDeclare("H(99)T2!C3V(SUM8)E(88)");
    differentInner.setVerifyFlags("TC");
    QList<QByteArray> finalContents;
    differentInner.registerRawType(7, [&](const QByteArray& bytes) { finalContents.append(bytes); });
    outer.registerRawType(MultiProtocolPacket::Type, [&](const QByteArray& bytes) {
        differentInner.appendBuffer(bytes);
    });
    outer.appendBuffer(wrap(QByteArray::fromHex("9900070102030d88")));
    QCOMPARE(finalContents, QList<QByteArray>{QByteArray::fromHex("010203")});
}

void MultiProtocolTest::mixedPartitions() {
    const QList<QByteArray> contents = {QByteArray::fromHex("0134"), QByteArray(),
        QByteArray::fromHex("00550066"), QByteArray::fromHex("0304")};
    const QList<QByteArray> frames = {independentFrame(contents[0]),
        independentFrame(contents[1], QByteArray::fromHex("aa66"), QByteArray::fromHex("0e0b"), 2),
        independentFrame(contents[2], QByteArray::fromHex("aa55"), QByteArray::fromHex("0e0b"), 1),
        independentFrame(contents[3], QByteArray::fromHex("aa55"), QByteArray::fromHex("0d0a"), 3)};
    QByteArray stream;
    QList<int> boundaries;
    for (const auto& frame : frames) { stream += frame; boundaries.append(stream.size()); }
    for (const bool metadata : {false, true}) {
        for (int a = 0; a <= stream.size(); ++a) {
            for (int b = a; b <= stream.size(); ++b) {
                receivedProtocols.clear();
                receivedContents.clear();
                MultiProtocolCodec codec;
                if (metadata) codec.enableRecordDecodeMetaData();
                configure(codec);
                int offset = 0;
                for (const int size : {a, b - a, int(stream.size()) - b}) {
                    codec.appendBuffer(stream.mid(offset, size));
                    offset += size;
                    int completed = 0;
                    for (const int boundary : boundaries) completed += boundary <= offset;
                    QCOMPARE(receivedProtocols, (QList<int>{1, 2, 3, 4}).mid(0, completed));
                    QCOMPARE(receivedContents, contents.mid(0, completed));
                }
                if (metadata) {
                    for (int id = 1; id <= 4; ++id) {
                        QCOMPARE(codec.getLastDecodeMetaData(id).content, contents[id - 1]);
                    }
                }
            }
        }
    }
}

void MultiProtocolTest::nestedFrameWait() {
    MultiProtocolCodec codec;
    configure(codec);
    const auto inner = independentFrame(QByteArray::fromHex("1234"), QByteArray::fromHex("aa66"),
        QByteArray::fromHex("0e0b"), 2);
    const QByteArray content = QByteArray::fromHex("9988") + inner + QByteArray::fromHex("7766");
    const auto outer = independentFrame(content);
    const int split = 6 + 2 + inner.size();
    codec.appendBuffer(outer.left(split));
    QVERIFY(receivedProtocols.isEmpty());
    codec.appendBuffer(outer.mid(split) + inner);
    QCOMPARE(receivedProtocols, (QList<int>{1, 2}));
    QCOMPARE(receivedContents, (QList<QByteArray>{content, QByteArray::fromHex("1234")}));
}

void MultiProtocolTest::badFramesRecover() {
    MultiProtocolCodec codec;
    configure(codec);
    const auto frame = independentFrame(QByteArray::fromHex("1234"));
    const auto good = independentFrame(QByteArray::fromHex("5678"), QByteArray::fromHex("aa66"),
        QByteArray::fromHex("0e0b"), 2);
    auto crc = frame;
    crc[crc.size() - 4] = char(uchar(crc.at(crc.size() - 4)) ^ 1);
    auto tail = frame;
    tail[tail.size() - 1] = char(0);
    auto oversized = frame;
    oversized[4] = char(0xff);
    oversized[5] = char(0xff);
    const auto wrongAddress = independentFrame(QByteArray::fromHex("1234"), QByteArray::fromHex("aa55"),
        QByteArray::fromHex("0d0a"), 127);
    const auto unknown = independentFrame(QByteArray::fromHex("1234"), QByteArray::fromHex("aa55"),
        QByteArray::fromHex("0d0a"), 1, 0x22);
    for (const auto& damaged : {crc, tail, oversized, wrongAddress, unknown}) {
        receivedProtocols.clear();
        codec.appendBuffer(QByteArray::fromHex("010203aa") + damaged + good);
        QCOMPARE(receivedProtocols, QList<int>{2});
    }
}

void MultiProtocolTest::sizeLessAndCommands() {
    for (const bool fixed : {false, true}) {
        MultiProtocolCodec codec;
        MultiProtocolCodec::ProtocolDefinition first;
        first.frameTemplate = fixed ? "H(AA55)T1C8V(CRC16)E(0D0A)" : "H(AA55)T1CV(CRC16)E(0D0A)";
        first.verifyFlags = "HTC";
        if (!fixed) first.contentSizeFromType = {{MultiProtocolPacket::Type, 8}};
        auto second = first;
        second.frameTemplate.replace("AA55", "AA66");
        codec.addProtocol(1, first);
        codec.addProtocol(2, second);
        codec.registerType<BytesCodec<MultiProtocolPacket>>(1, this, &MultiProtocolTest::receiveOne);
        codec.registerType<BytesCodec<MultiProtocolPacket>>(2, this, &MultiProtocolTest::receiveTwo);
        const QByteArray content = QByteArray::fromHex("0102030405060708");
        const auto stream = codec.encode(1, MultiProtocolPacket{content}) + codec.encode(2, MultiProtocolPacket{content});
        receivedProtocols.clear();
        for (const char byte : stream) codec.appendBuffer(QByteArray(1, byte));
        QCOMPARE(receivedProtocols, (QList<int>{1, 2}));
    }
    MultiProtocolCodec commands;
    commands.addProtocol(1, "H(AA55)T1E(0D0A)");
    commands.addProtocol(2, "H(AA66)T1E(0D0A)");
    commands.registerType<7>(1, this, &MultiProtocolTest::commandOne);
    commands.registerType<7>(2, this, &MultiProtocolTest::commandTwo);
    receivedProtocols.clear();
    commands.appendBuffer(commands.encode<7>(2) + commands.encode<7>(1));
    QCOMPARE(receivedProtocols, (QList<int>{2, 1}));
}

void MultiProtocolTest::layoutVariants() {
    const QStringList formats = {
        "H(AA55)S2T1CV(CRC16)E(0D0A)",
        "H(AA55)S2CT1V(CRC16)E(0D0A)",
        "H(AA55)T1S2CE(0D0A)V(CRC16)",
        "H(AA55)T1S2C",
        "H(AA55)C8T1V(CRC16)E(0D0A)",
        "H(AA55)T1C",
        "H(AA55)T2!S2!CV!(CRC16)E(0D0A)"};
    for (int index = 0; index < formats.size(); ++index) {
        MultiProtocolCodec codec;
        MultiProtocolCodec::ProtocolDefinition first;
        first.frameTemplate = formats[index];
        first.verifyFlags = index == 3 || index == 5 ? "" : index == 2 ? "STCE" : "TC";
        if (first.frameTemplate.contains('S')) first.sizeFlags = "TC";
        else if (index == 5) first.contentSizeFromType = {{MultiProtocolPacket::Type, 8}};
        auto second = first;
        second.frameTemplate.replace("AA55", "AA66");
        codec.addProtocol(1, first);
        codec.addProtocol(2, second);
        codec.registerType<BytesCodec<MultiProtocolPacket>>(1, this, &MultiProtocolTest::receiveOne);
        codec.registerType<BytesCodec<MultiProtocolPacket>>(2, this, &MultiProtocolTest::receiveTwo);
        const QByteArray content = QByteArray::fromHex("0102030405060708");
        const auto stream = codec.encode(2, MultiProtocolPacket{content}) + codec.encode(1, MultiProtocolPacket{content});
        receivedProtocols.clear();
        receivedContents.clear();
        for (const char byte : stream) codec.appendBuffer(QByteArray(1, byte));
        QCOMPARE(receivedProtocols, (QList<int>{2, 1}));
        QCOMPARE(receivedContents, (QList<QByteArray>{content, content}));
    }
}

void MultiProtocolTest::loggingFlags() {
    LogCapture capture;
    MultiProtocolCodec codec;
    codec.setLogging(multiProtocolTestLog);
    configure(codec);
    const auto good = independentFrame(QByteArray::fromHex("1234"));
    auto bad = good;
    bad[bad.size() - 4] = char(uchar(bad.at(bad.size() - 4)) ^ 1);
    codec.appendBuffer(bad + good);
    QVERIFY(capture.messages.isEmpty());
    receivedProtocols.clear();
    codec.setDecodeErrorLogFlags(DecodeError::ChecksumMismatch);
    codec.appendBuffer(bad + good);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=checksum-mismatch"));
    QVERIFY(capture.messages.first().contains("protocol=1"));
    QVERIFY(capture.messages.first().contains("boundary=complete-frame"));
    QVERIFY(capture.messages.first().contains(QString::fromLatin1(bad.toHex(' '))));
    QCOMPARE(receivedProtocols, QList<int>{1});
    capture.messages.clear();
    codec.appendBuffer(independentFrame(QByteArray::fromHex("1234"), QByteArray::fromHex("aa55"),
        QByteArray::fromHex("0e0b"), 1));
    QVERIFY(capture.messages.isEmpty());
}

void MultiProtocolTest::timeoutAndCacheLimit() {
    MultiProtocolCodec codec;
    configure(codec);
    const auto first = independentFrame(QByteArray::fromHex("1234"));
    const auto second = independentFrame(QByteArray::fromHex("5678"), QByteArray::fromHex("aa66"),
        QByteArray::fromHex("0e0b"), 2);
    codec.setDecodeTimeout(20);
    codec.appendBuffer(first.left(4));
    QTest::qSleep(40);
    codec.appendBuffer(second);
    QCOMPARE(receivedProtocols, QList<int>{2});
    receivedProtocols.clear();
    codec.setDecodeTimeout(-1);
    codec.setBufferMaxSize(32);
    auto oversized = first.left(6);
    oversized[4] = char(100);
    oversized[5] = char(0);
    codec.appendBuffer(oversized + second);
    QCOMPARE(receivedProtocols, QList<int>{2});
    LogCapture capture;
    codec.setLogging(multiProtocolTestLog);
    codec.setDecodeErrorLogFlags(DecodeError::BufferOverflow);
    codec.setBufferMaxSize(1);
    codec.appendBuffer(first.left(2));
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=buffer-overflow"));
    codec.clearDecodeCacheBuffer();
    codec.setBufferMaxSize(32);
    codec.appendBuffer(second);
    QCOMPARE(receivedProtocols, (QList<int>{2, 2}));
}

void MultiProtocolTest::reentrantAndClear() {
    MultiProtocolCodec codec;
    configure(codec);
    const auto first = independentFrame(QByteArray::fromHex("1234"));
    const auto second = independentFrame(QByteArray::fromHex("5678"), QByteArray::fromHex("aa66"),
        QByteArray::fromHex("0e0b"), 2);
    const auto third = independentFrame(QByteArray::fromHex("90ab"), QByteArray::fromHex("aa55"),
        QByteArray::fromHex("0e0b"), 1);
    onReceive = [&](int id) { if (id == 1) codec.appendBuffer(second); };
    codec.appendBuffer(first + third);
    QCOMPARE(receivedProtocols, (QList<int>{1, 3, 2}));
    receivedProtocols.clear();
    onReceive = [&](int id) {
        if (id == 1) {
            codec.appendBuffer(third);
            codec.clearDecodeCacheBuffer();
            codec.appendBuffer(second);
        }
    };
    codec.appendBuffer(first + third);
    QCOMPARE(receivedProtocols, (QList<int>{1, 2}));
    QVERIFY_EXCEPTION_THROWN(codec.addProtocol(1, definition()), ProtocolException);
}

void MultiProtocolTest::callbackException() {
    MultiProtocolCodec codec;
    configure(codec);
    const auto first = independentFrame(QByteArray::fromHex("1234"));
    const auto second = independentFrame(QByteArray::fromHex("5678"), QByteArray::fromHex("aa66"),
        QByteArray::fromHex("0e0b"), 2);
    onReceive = [](int id) { if (id == 1) throw std::runtime_error("test callback failure"); };
    QVERIFY_EXCEPTION_THROWN(codec.appendBuffer(first + second), std::runtime_error);
    onReceive = {};
    codec.appendBuffer(second);
    QCOMPARE(receivedProtocols, (QList<int>{1, 2, 2}));
}

void MultiProtocolTest::metadataIsolation() {
    MultiProtocolCodec codec;
    configure(codec);
    codec.enableRecordDecodeMetaData();
    const QByteArray first = QByteArray::fromHex("1234");
    const QByteArray second = QByteArray::fromHex("5678");
    onReceive = [&](int id) {
        const auto metadata = codec.getLastDecodeMetaData(id);
        payloadsMatch = payloadsMatch && metadata.content == (id == 1 ? first : second)
            && codec.lastDecodedProtocolId() == id && metadata.verify == metadata.localVerifyCode;
    };
    codec.appendBuffer(codec.encode(1, MultiProtocolPacket{first}) + codec.encode(2, MultiProtocolPacket{second}));
    QVERIFY(payloadsMatch);
    QCOMPARE(codec.getLastDecodeMetaData(1).header, QByteArray::fromHex("aa55"));
    QCOMPARE(codec.getLastDecodeMetaData(2, MultiProtocolPacket::Type).header, QByteArray::fromHex("aa66"));
    QCOMPARE(codec.getLastDecodeMetaData(1).content, first);
    QCOMPARE(codec.getLastDecodeMetaData(2).content, second);
    QCOMPARE(codec.lastDecodedProtocolId(), 2);
}

void MultiProtocolTest::hundredMiBStream() {
    MultiProtocolCodec codec;
    configure(codec);
    recordEvents = false;
    expectedContents = {{1, QByteArray(64, '1')}, {2, QByteArray(64, '2')}};
    const auto pair = independentFrame(expectedContents[1]) + independentFrame(expectedContents[2],
        QByteArray::fromHex("aa66"), QByteArray::fromHex("0e0b"), 2);
    QByteArray stream;
    stream.reserve(100 * 1024 * 1024 + pair.size());
    while (stream.size() < 100 * 1024 * 1024) stream += pair;
    QElapsedTimer timer;
    timer.start();
    for (int offset = 0; offset < stream.size(); offset += 65536) codec.appendBuffer(stream.mid(offset, 65536));
    const auto milliseconds = timer.elapsed();
    QVERIFY(payloadsMatch);
    QCOMPARE(receivedCounts.value(1), quint64(stream.size() / pair.size()));
    QCOMPARE(receivedCounts.value(2), quint64(stream.size() / pair.size()));
    QCOMPARE(receivedCounts.value(3), quint64(0));
    QCOMPARE(receivedCounts.value(4), quint64(0));
    qInfo().nospace() << "MULTI_STREAM_RESULT bytes=" << stream.size() << " elapsed_ms=" << milliseconds
        << " callbacks=" << (receivedCounts.value(1) + receivedCounts.value(2));
}

void MultiProtocolTest::receiveOne(const MultiProtocolPacket& packet) { receive(1, packet); }
void MultiProtocolTest::receiveTwo(const MultiProtocolPacket& packet) { receive(2, packet); }
void MultiProtocolTest::receiveThree(const MultiProtocolPacket& packet) { receive(3, packet); }
void MultiProtocolTest::receiveFour(const MultiProtocolPacket& packet) { receive(4, packet); }

void MultiProtocolTest::receive(int protocol, const MultiProtocolPacket& packet) {
    ++receivedCounts[protocol];
    if (recordEvents) {
        receivedProtocols.append(protocol);
        receivedContents.append(packet.content);
    }
    if (expectedContents.contains(protocol)) payloadsMatch = payloadsMatch && packet.content == expectedContents[protocol];
    if (onReceive) onReceive(protocol);
}

void MultiProtocolTest::commandOne() { receivedProtocols.append(1); }
void MultiProtocolTest::commandTwo() { receivedProtocols.append(2); }

void MultiProtocolTest::cleanup() {
    receivedProtocols.clear();
    receivedContents.clear();
    receivedCounts.clear();
    expectedContents.clear();
    onReceive = {};
    recordEvents = true;
    payloadsMatch = true;
}
