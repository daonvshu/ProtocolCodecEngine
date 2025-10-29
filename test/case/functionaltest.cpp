#include "functionaltest.h"
#include "byteutils.h"

#include <qtest.h>

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

void FunctionalTest::cleanup() {
    lastDecodedData.clear();
    codec.clearDecodeCacheBuffer();
}

void FunctionalTest::cleanupTestCase() {
}

void FunctionalTest::callback(const FunctionalTestData& data) {
    lastDecodedData << data;
}
