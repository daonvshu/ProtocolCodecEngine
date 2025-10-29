#include "filetest.h"

#include <qfile.h>
#include <qelapsedtimer.h>
#include <qtest.h>

Q_LOGGING_CATEGORY(fileTest, "file_test")

FileTest::FileTest(QObject* parent)
    : QObject(parent) {
}

void FileTest::initTestCase() {
    codec.frameDeclare("H(5AFF)S2T2CV(SUM16)E(FE)");
    // codecEngine.setAddressValue(0);
    codec.setVerifyFlags("STC");
    codec.setSizeFlags("TC");
    codec.registerType<BytesCodec<FrameData>>(this, &FileTest::callbackFunc);
    codec.setBufferMaxSize(sizeof(FrameData) + 9);
    codec.setSizeMaxValue(sizeof(FrameData) + 2);
    //codecEngine.setDecodeTimeout(10);
    //codec.setLogging(fileTest);
}

void FileTest::decodeFromFileTest() {
    objSize = 0;

    QFile file("1.bin");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "skip file test!";
        return;
    }

    QElapsedTimer timer;
    timer.start();
    while (!file.atEnd()) {
        auto data = file.read(102400);
        codec.appendBuffer(data);
    }
    qDebug() << "decode used time:" << timer.elapsed() << " decoded object size:" << objSize;
    file.close();

    QCOMPARE(9160, objSize);
}

void FileTest::cleanup() {
}

void FileTest::cleanupTestCase() {
}

void FileTest::callbackFunc(const FrameData& data) {
    objSize++;
}
