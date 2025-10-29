#include <QCoreApplication>

#include "multitestrunner.h"

#include "case/functionaltest.h"
#include "case/flagtest.h"
#include "case/codectest.h"
#include "case/filetest.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

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