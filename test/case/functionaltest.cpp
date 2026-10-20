#include "functionaltest.h"
#include "byteutils.h"

#include <qtest.h>
#include <protocolcodec/flagdata/datacontent.h>
#include <protocolcodec/flagdata/dataverify.h>
#include <protocolcodec/verifier/crccheckverify.h>

Q_LOGGING_CATEGORY(decodeErrorTestLog, "protocol.decode.test")

namespace {
class DecodeLogCapture {
public:
    DecodeLogCapture() : previous(qInstallMessageHandler(handleMessage)) { active = this; }
    ~DecodeLogCapture() { qInstallMessageHandler(previous); active = nullptr; }

    QStringList messages;
    QList<QtMsgType> levels;

private:
    static void handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
        if (active != nullptr && QByteArray(context.category) == "protocol.decode.test") {
            active->messages.append(message);
            active->levels.append(type);
        } else if (active != nullptr && active->previous != nullptr) {
            active->previous(type, context, message);
        }
    }

    QtMessageHandler previous;
    static DecodeLogCapture* active;
};
DecodeLogCapture* DecodeLogCapture::active = nullptr;
}

static const QByteArray testData1 = TByteUtils::make({ 0x5a, 0xfa, 0x64, 0x0a, 0x09, 0x00, 0x34, 0x12, 0x00, 0x00, 0x78, 0x56, 0x00, 0x00, 0x2c, 0xec, 0xfe, 0xef });
static const QByteArray testData2 = TByteUtils::make({ 0x5a, 0xfa, 0x64, 0x0a, 0x09, 0x00, 0x44, 0x22, 0x00, 0x00, 0x88, 0x66, 0x00, 0x00, 0x28, 0xc4, 0xfe, 0xef });
static const QByteArray testData3 = TByteUtils::make({ 0x5a, 0xfa, 0x64, 0x0a, 0x09, 0x00, 0x54, 0x32, 0x00, 0x00, 0x98, 0x76, 0x00, 0x00, 0x3d, 0x0c, 0xfe, 0xef });

FunctionalTest::FunctionalTest(QObject* parent)
    : QObject(parent) {
}

void FunctionalTest::initTestCase() {
    codec.frameDeclare("H(5AFA)T1A1S2CV(CRC16)E(FEEF)");
    codec.setVerifyFlags("STAC");
    codec.setSizeFlags("TC");
    codec.setAddressValue(10);
    codec.enableRecordDecodeMetaData();

    codec.registerType<BytesCodec<FunctionalTestData>>(this, &FunctionalTest::callback);
}

void FunctionalTest::encodeTest() {
    FunctionalTestData data;
    data.data1 = 0x1234;
    data.data2 = 0x5678;
    auto bytes = codec.encode(data);
    qDebug() << "encode bytes:" << bytes.toHex(' ');
    QCOMPARE(bytes.size(), 10 + sizeof(FunctionalTestData));

    auto metaData = codec.getLastEncodeMetaData();
    QCOMPARE(metaData.header, TByteUtils::make({ 0x5A, 0xFA }));
    QCOMPARE(metaData.address, TByteUtils::make({ 10 }));
    QCOMPARE(metaData.type, TByteUtils::make({ FunctionalTestData::Type }));
    QCOMPARE(metaData.size, TByteUtils::make({ 1 + sizeof(FunctionalTestData), 0x00 }));
    QCOMPARE(metaData.content, TByteUtils::make({ 0x34, 0x12, 0x00, 0x00, 0x78, 0x56, 0x00, 0x00 }));
    QCOMPARE(metaData.verify, TByteUtils::make({ 0x2C, 0xEC }));
    QCOMPARE(metaData.end, TByteUtils::make({ 0xFE, 0xEF }));
}

void FunctionalTest::decodeTest() {
    codec.appendBuffer(testData1);

    auto metaData = codec.getLastDecodeMetaData();
    QCOMPARE(metaData.header, TByteUtils::make({ 0x5A, 0xFA }));
    QCOMPARE(metaData.address, TByteUtils::make({ 10 }));
    QCOMPARE(metaData.type, TByteUtils::make({ FunctionalTestData::Type }));
    QCOMPARE(metaData.size, TByteUtils::make({ 1 + sizeof(FunctionalTestData), 0x00 }));
    QCOMPARE(metaData.content, TByteUtils::make({ 0x34, 0x12, 0x00, 0x00, 0x78, 0x56, 0x00, 0x00 }));
    QCOMPARE(metaData.verify, TByteUtils::make({ 0x2C, 0xEC }));
    QCOMPARE(metaData.end, TByteUtils::make({ 0xFE, 0xEF }));

    QCOMPARE(lastDecodedData.size(), 1);

    QCOMPARE(lastDecodedData.first().data1, 0x1234);
    QCOMPARE(lastDecodedData.first().data2, 0x5678);
}

void FunctionalTest::garbageDecodeTest() {
    auto garbage1 = TByteUtils::makeRandomBytes(20);
    garbage1.insert(3, TByteUtils::make({ 0x5a, 0xfa }));
    auto garbage2 = TByteUtils::makeRandomBytes(15);
    garbage2.insert(5, TByteUtils::make({ 0x5a, 0xfa, 0x64 }));
    auto garbage3 = TByteUtils::makeRandomBytes(10);
    garbage3.insert(7, TByteUtils::make({ 0x5a, 0xfa, 0x64, 0x0a }));
    auto garbage4 = TByteUtils::makeRandomBytes(15);
    garbage4.insert(8, TByteUtils::make({ 0x5a, 0xfa, 0x64, 0x0a, 0x09 }));

    codec.appendBuffer(garbage1);
    codec.appendBuffer(testData1);
    codec.appendBuffer(garbage2);
    auto data2 = testData2;
    data2.replace(15, 2, TByteUtils::make({ 0x0A, 0x0B }));
    codec.appendBuffer(data2);
    codec.appendBuffer(garbage3);
    codec.appendBuffer(testData3);
    codec.appendBuffer(garbage4);

    QCOMPARE(lastDecodedData.size(), 2);

    QCOMPARE(lastDecodedData[0].data1, 0x1234);
    QCOMPARE(lastDecodedData[0].data2, 0x5678);
    QCOMPARE(lastDecodedData[1].data1, 0x3254);
    QCOMPARE(lastDecodedData[1].data2, 0x7698);
}

void FunctionalTest::frameBreakTest() {
    auto dataPart1 = testData1.left(10);
    auto dataPart2 = testData1.mid(10);
    codec.appendBuffer(dataPart1);
    codec.appendBuffer(dataPart2);

    QCOMPARE(lastDecodedData.size(), 1);
    QCOMPARE(lastDecodedData[0].data1, 0x1234);
    QCOMPARE(lastDecodedData[0].data2, 0x5678);

    auto garbage = TByteUtils::makeRandomBytes(20);
    dataPart1 = testData2.left(1);
    dataPart2 = testData2.mid(1, testData2.size() - 3);
    auto dataPart3 = testData2.right(2);
    codec.appendBuffer(garbage);
    auto data2 = testData2;
    data2.replace(15, 2, TByteUtils::make({ 0x0A, 0x0B }));
    codec.appendBuffer(data2);
    codec.appendBuffer(dataPart1);
    codec.appendBuffer(dataPart2);
    codec.appendBuffer(dataPart3);

    QCOMPARE(lastDecodedData.size(), 2);
    QCOMPARE(lastDecodedData[1].data1, 0x2244);
    QCOMPARE(lastDecodedData[1].data2, 0x6688);
}

void FunctionalTest::decodeErrorKindsTest_data() {
    QTest::addColumn<QByteArray>("badFrame");
    QTest::addColumn<int>("errorFlag");
    QTest::addColumn<QString>("errorName");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<bool>("completeFrame");

    auto bytes = testData1;
    bytes[17] = char(0);
    QTest::newRow("tail") << bytes << int(DecodeError::TailMismatch)
        << QString("tail-mismatch") << QString("expected=fe ef received=fe 00") << true;
    bytes = testData1;
    bytes[14] = char(0);
    QTest::newRow("crc") << bytes << int(DecodeError::ChecksumMismatch)
        << QString("checksum-mismatch") << QString("calculated=2c ec received=00 ec") << true;
    bytes = testData1;
    bytes[3] = char(11);
    QTest::newRow("address") << bytes << int(DecodeError::AddressMismatch)
        << QString("address-mismatch") << QString("expected=0a received=0b") << true;
    bytes = testData1;
    bytes[4] = char(0xff);
    QTest::newRow("oversize") << bytes << int(DecodeError::InvalidLength)
        << QString("invalid-length") << QString("declared=255 maximum=32") << false;
    bytes = testData1;
    bytes[4] = char(0);
    QTest::newRow("undersize") << bytes << int(DecodeError::InvalidLength)
        << QString("invalid-length") << QString("invalid or unknown content length") << false;
    bytes = testData1;
    bytes[2] = char(101);
    CrcCheckVerify(2).getVerifyCode(bytes.mid(2, 12), bytes.data() + 14, true);
    QTest::newRow("unknown-type") << bytes << int(DecodeError::UnknownType)
        << QString("unknown-type") << QString("no decoder registered for type=0x65") << true;
}

void FunctionalTest::decodeErrorKindsTest() {
    QFETCH(QByteArray, badFrame);
    QFETCH(int, errorFlag);
    QFETCH(QString, errorName);
    QFETCH(QString, detail);
    QFETCH(bool, completeFrame);
    codec.setLogging(decodeErrorTestLog);
    codec.setDecodeErrorLogFlags(DecodeError(errorFlag));
    codec.setSizeMaxValue(32);
    DecodeLogCapture capture;
    codec.appendBuffer(badFrame + testData1);
    QCOMPARE(lastDecodedData.size(), 1);
    QCOMPARE(lastDecodedData.first().data1, 0x1234);
    QCOMPARE(capture.messages.size(), 1);
    const auto message = capture.messages.first();
    QVERIFY2(message.contains("error=" + errorName), qPrintable(message));
    QVERIFY2(message.contains(detail), qPrintable(message));
    QVERIFY(message.contains(completeFrame ? "boundary=complete-frame" : "boundary=unknown"));
    QVERIFY(message.contains("bytes=18 hex=" + QString::fromLatin1(badFrame.toHex(' '))));
    QVERIFY(!message.contains(QString::fromLatin1((badFrame + testData1).toHex(' '))));
    QCOMPARE(capture.levels.first(), QtWarningMsg);
}

void FunctionalTest::decodeErrorFlagsTest() {
    ProtocolCodecEngine parser;
    parser.frameDeclare("H(5AFA)T1A1S2CV(CRC16)E(FEEF)");
    parser.setVerifyFlags("STAC");
    parser.setSizeFlags("TC");
    parser.setAddressValue(10);
    parser.registerType<BytesCodec<FunctionalTestData>>(this, &FunctionalTest::callback);
    parser.setLogging(decodeErrorTestLog);
    auto badCrc = testData1;
    badCrc[14] = char(0);
    auto badTail = testData1;
    badTail[17] = char(0);
    DecodeLogCapture capture;
    parser.setDecodeErrorLogFlags(DecodeError::None);
    parser.appendBuffer(badCrc + badTail + testData1);
    QVERIFY(capture.messages.isEmpty());
    QCOMPARE(lastDecodedData.size(), 1);
    parser.setDecodeErrorLogFlags(DecodeError::TailMismatch);
    parser.appendBuffer(badCrc);
    QVERIFY(capture.messages.isEmpty());
    parser.appendBuffer(badTail);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=tail-mismatch"));
    capture.messages.clear();
    parser.setDecodeErrorLogFlags(DecodeError::TailMismatch | DecodeError::ChecksumMismatch);
    parser.appendBuffer(badTail + badCrc);
    QCOMPARE(capture.messages.size(), 2);
    QVERIFY(capture.messages.last().contains("calculated=2c ec received=00 ec"));
}

void FunctionalTest::partialFrameErrorLogTest() {
    codec.setLogging(decodeErrorTestLog);
    codec.setDecodeErrorLogFlags(DecodeError::All);
    DecodeLogCapture capture;
    for (int split = 1; split < testData1.size(); ++split) {
        codec.appendBuffer(testData1.left(split));
        QVERIFY2(capture.messages.isEmpty(), qPrintable(capture.messages.join('\n')));
        codec.appendBuffer(testData1.mid(split));
        QVERIFY2(capture.messages.isEmpty(), qPrintable(capture.messages.join('\n')));
        QCOMPARE(lastDecodedData.size(), split);
    }
    codec.setSizeMaxValue(32);
    auto embeddedHeader = testData1;
    embeddedHeader.replace(6, 8, QByteArray::fromHex("5afa640affff0000"));
    CrcCheckVerify(2).getVerifyCode(embeddedHeader.mid(2, 12), embeddedHeader.data() + 14, true);
    codec.appendBuffer(embeddedHeader.left(17));
    QVERIFY2(capture.messages.isEmpty(), qPrintable(capture.messages.join('\n')));
    codec.appendBuffer(embeddedHeader.mid(17));
    QVERIFY2(capture.messages.isEmpty(), qPrintable(capture.messages.join('\n')));
    QCOMPARE(lastDecodedData.size(), 18);
    auto badTail = testData1;
    badTail[17] = char(0);
    codec.appendBuffer(badTail.left(10));
    QVERIFY(capture.messages.isEmpty());
    codec.appendBuffer(badTail.mid(10));
    QCOMPARE(capture.messages.size(), 1);
    codec.appendBuffer(testData2);
    QCOMPARE(capture.messages.size(), 1);
    QCOMPARE(lastDecodedData.last().data1, 0x2244);
}

void FunctionalTest::cacheDiscardErrorLogTest() {
    codec.setLogging(decodeErrorTestLog);
    codec.setDecodeErrorLogFlags(DecodeError::Timeout | DecodeError::BufferOverflow);
    codec.setDecodeTimeout(10);
    DecodeLogCapture capture;
    codec.appendBuffer(testData1.left(10));
    QVERIFY(capture.messages.isEmpty());
    QTest::qWait(30);
    codec.appendBuffer(testData2);
    QCOMPARE(lastDecodedData.size(), 1);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=timeout"));
    QVERIFY(capture.messages.first().contains("bytes=10 hex="
        + QString::fromLatin1(testData1.left(10).toHex(' '))));
    codec.setDecodeTimeout(-1);
    codec.setBufferMaxSize(8);
    capture.messages.clear();
    codec.appendBuffer(testData1.left(12));
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=buffer-overflow"));
    QVERIFY(capture.messages.first().contains("bytes=4 hex="
        + QString::fromLatin1(testData1.left(4).toHex(' '))));
    codec.appendBuffer(testData3);
    QCOMPARE(lastDecodedData.size(), 2);
    QCOMPARE(capture.messages.size(), 1);
    codec.setBufferMaxSize(10 * 1024 * 1024);
    codec.appendBuffer(testData1.left(10));
    codec.clearDecodeCacheBuffer();
    codec.appendBuffer(testData1);
    QCOMPARE(lastDecodedData.size(), 3);
}

void FunctionalTest::largeRejectedFrameLogTest() {
    codec.setLogging(decodeErrorTestLog);
    codec.setDecodeErrorLogFlags(DecodeError::TailMismatch);
    auto frame = testData1.left(6) + QByteArray(4096, 'a') + QByteArray::fromHex("0000fe00");
    frame[4] = char(0x01);
    frame[5] = char(0x10);
    DecodeLogCapture capture;
    codec.appendBuffer(frame + testData1);
    QCOMPARE(lastDecodedData.size(), 1);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("bytes=4106 hex=" + QString::fromLatin1(frame.toHex(' '))));
    QVERIFY(!capture.messages.first().contains(QString::fromLatin1((frame + testData1).toHex(' '))));
}

void FunctionalTest::framingErrorLogTest() {
    codec.setLogging(decodeErrorTestLog);
    codec.setDecodeErrorLogFlags(DecodeError::All);
    DecodeLogCapture capture;
    const auto noise = QByteArray::fromHex("010203");
    codec.appendBuffer(noise + testData1.left(1));
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=header-mismatch"));
    QVERIFY(capture.messages.first().contains("bytes=3 hex=01 02 03"));
    codec.appendBuffer(testData1.mid(1));
    QCOMPARE(lastDecodedData.size(), 1);
    QCOMPARE(capture.messages.size(), 1);

    // A damaged length must not hide a complete valid frame later in the same buffer.
    auto incomplete = testData1.left(10);
    incomplete[4] = char(30);
    codec.appendBuffer(incomplete + testData2);
    QCOMPARE(lastDecodedData.size(), 2);
    QCOMPARE(lastDecodedData.last().data1, 0x2244);
}

void FunctionalTest::contentLengthErrorTest() {
    ProtocolCodecEngine parser;
    parser.frameDeclare("H(5AFA)T1A1C");
    parser.setAddressValue(10);
    parser.setContentSizeDependsOnType({{100, 8}});
    parser.registerType<BytesCodec<FunctionalTestData>>(this, &FunctionalTest::callback);
    parser.setLogging(decodeErrorTestLog);
    parser.setDecodeErrorLogFlags(DecodeError::All);
    DecodeLogCapture capture;
    const auto frame = testData1.left(4) + testData1.mid(6, 8);
    parser.appendBuffer(frame.left(7));
    QVERIFY(lastDecodedData.isEmpty());
    QVERIFY(capture.messages.isEmpty());
    parser.appendBuffer(frame.mid(7));
    QCOMPARE(lastDecodedData.size(), 1);
    QVERIFY(capture.messages.isEmpty());
    auto unknown = frame;
    unknown[2] = char(101);
    parser.appendBuffer(unknown + frame);
    QCOMPARE(lastDecodedData.size(), 2);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=unknown-type"));

    parser.frameDeclare("H(5AFA)T1S4CE(FEEF)");
    capture.messages.clear();
    parser.appendBuffer(QByteArray::fromHex("5afa64ffffff7f"));
    QVERIFY(lastDecodedData.size() == 2);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("error=invalid-length"));
}

void FunctionalTest::crcValueLogTest() {
    ProtocolFlagDataContent content(9);
    content.isVerifyTarget = true;
    ProtocolFlagDataVerify verify(ProtocolFlagDataVerify::Crc16);
    verify.bindPrev(&content);
    const QByteArray payload("123456789");
    const QByteArray expected = verify.getVerifyCode(payload, payload.size());
    auto frame = payload + expected;
    frame[9] = char(frame.at(9) ^ 0xff);
    ProtocolMetaData metadata;
    DecodeLogCapture capture;
    QVERIFY(!verify.verify(frame.data(), 0, frame.size(), &metadata, decodeErrorTestLog));
    QCOMPARE(metadata.localVerifyCode, expected);
    QCOMPARE(capture.messages.size(), 1);
    QVERIFY(capture.messages.first().contains("calc code: " + QString::fromLatin1(expected.toHex())));
    QVERIFY(capture.messages.first().contains("data code: " + QString::fromLatin1(frame.right(2).toHex())));
}

void FunctionalTest::cleanup() {
    lastDecodedData.clear();
    codec.clearDecodeCacheBuffer();
    codec.setLogging(nullptr);
    codec.setDecodeErrorLogFlags(DecodeError::All);
    codec.setSizeMaxValue(-1);
    codec.setBufferMaxSize(10 * 1024 * 1024);
    codec.setDecodeTimeout(-1);
}

void FunctionalTest::cleanupTestCase() {
}

void FunctionalTest::callback(const FunctionalTestData& data) {
    lastDecodedData << data;
}
