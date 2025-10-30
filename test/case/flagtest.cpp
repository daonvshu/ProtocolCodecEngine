#include "flagtest.h"
#include "byteutils.h"

#include "protocolcodecengine.h"
#include <qtest.h>

PROTOCOL_CODEC_USING_NAMESPACE

FlagTest::FlagTest(QObject* parent)
    : QObject(parent) {
}

void FlagTest::initTestCase() {
}

void FlagTest::bigEndianTest() {
    ProtocolCodecEngine codec;

    codec.frameDeclare("H(5AFA)T2!A!2S2!CV!(CRC16)E(FEEF)");
    codec.setVerifyFlags("STAC");
    codec.setSizeFlags("TC");
    codec.setAddressValue(10);
    codec.enableRecordDecodeMetaData();

    codec.registerType<BytesCodec<FunctionalTestData>>(this, &FlagTest::callback);

    FunctionalTestData data;
    data.data1 = 0x1234;
    data.data2 = 0x5678;
    auto bytes = codec.encode(data);
    qDebug() << "encode bytes:" << bytes.toHex(' ');
    QCOMPARE(bytes.size(), 12 + sizeof(FunctionalTestData));

    auto metaData = codec.getLastEncodeMetaData();
    QCOMPARE(metaData.header, TByteUtils::make({ 0x5A, 0xFA }));
    QCOMPARE(metaData.address, TByteUtils::make({ 0, 10 }));
    QCOMPARE(metaData.type, TByteUtils::make({ 0x00, FunctionalTestData::Type }));
    QCOMPARE(metaData.size, TByteUtils::make({ 0x00, 2 + sizeof(FunctionalTestData) }));
    QCOMPARE(metaData.content, TByteUtils::make({ 0x34, 0x12, 0x00, 0x00, 0x78, 0x56, 0x00, 0x00 }));
    QCOMPARE(metaData.verify, TByteUtils::make({ 0x0B, 0xB3 }));
    QCOMPARE(metaData.end, TByteUtils::make({ 0xFE, 0xEF }));

    codec.appendBuffer(bytes);
    QCOMPARE(lastDecodedData.size(), 1);

    metaData = codec.getLastDecodeMetaData();
    QCOMPARE(metaData.header, TByteUtils::make({ 0x5A, 0xFA }));
    QCOMPARE(metaData.address, TByteUtils::make({ 0, 10 }));
    QCOMPARE(metaData.type, TByteUtils::make({ 0x00, FunctionalTestData::Type }));
    QCOMPARE(metaData.size, TByteUtils::make({ 0x00, 2 + sizeof(FunctionalTestData) }));
    QCOMPARE(metaData.content, TByteUtils::make({ 0x34, 0x12, 0x00, 0x00, 0x78, 0x56, 0x00, 0x00 }));
    QCOMPARE(metaData.verify, TByteUtils::make({ 0x0B, 0xB3 }));
    QCOMPARE(metaData.end, TByteUtils::make({ 0xFE, 0xEF }));

    QCOMPARE(lastDecodedData.first().data1, 0x1234);
    QCOMPARE(lastDecodedData.first().data2, 0x5678);
}

void FlagTest::cleanup() {
}

void FlagTest::cleanupTestCase() {
}

void FlagTest::callback(const FunctionalTestData& data) {
    lastDecodedData << data;
}
