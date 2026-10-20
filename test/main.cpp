#include <QCoreApplication>

#include "multitestrunner.h"

#include "case/functionaltest.h"
#include "case/flagtest.h"
#include "case/codectest.h"
#include "case/filetest.h"
#include "case/multiprotocoltest.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    if (a.arguments().contains("--multi-protocol-tests")) {
        MultiProtocolTest test;
        const int result = QTest::qExec(&test, QStringList{a.arguments().first(), "-o", "multi-protocol-tests.txt"});
        TestUtils::printTestLog("multi-protocol-tests.txt");
        return result;
    }

    if (a.arguments().contains("--decode-error-tests")) {
        FunctionalTest test;
        int result = QTest::qExec(&test, QStringList{a.arguments().first(), "decodeTest", "garbageDecodeTest",
            "frameBreakTest", "decodeErrorKindsTest", "decodeErrorFlagsTest", "partialFrameErrorLogTest",
            "largeRejectedFrameLogTest", "cacheDiscardErrorLogTest", "framingErrorLogTest",
            "contentLengthErrorTest", "crcValueLogTest", "-o", "decode-error-functional.txt"});
        TestUtils::printTestLog("decode-error-functional.txt");
        FlagTest flags;
        result += QTest::qExec(&flags, QStringList{a.arguments().first(), "bigEndianTest",
            "-o", "decode-error-endian.txt"});
        TestUtils::printTestLog("decode-error-endian.txt");
        return result;
    }

    int result = MultiTestRunner<
        FunctionalTest,
        FlagTest,
        CodecTest,
        FileTest
    >::run("protocol_codec_engine_test.exe");

    if (result != 0) {
        TestUtils::printWithColor("Not all tests are successful!", TestOutputColorAttr::Yellow);
    } else {
        TestUtils::printWithColor("All tests passed!", TestOutputColorAttr::Green);
    }
    return result;
}
