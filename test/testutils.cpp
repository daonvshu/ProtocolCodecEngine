#include "testutils.h"

#include <qfile.h>
#include <iostream>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

void TestUtils::printTestLog(const QString &filePath) {
    QFile file(filePath);
    Q_ASSERT(file.open(QIODevice::ReadOnly));
    while (true) {
        auto line = file.readLine();
        if (line.isEmpty()) break;
        if (line.startsWith("PASS")) {
            printWithColor(line, TestOutputColorAttr::Green, false);
        } else if (line.startsWith("FAIL")) {
            printWithColor(line, TestOutputColorAttr::Red, false);
        } else if (line.startsWith("Totals")) {
            printWithColor(line, TestOutputColorAttr::Yellow, false);
        } else if (line.startsWith("*********")) {
            printWithColor(line, TestOutputColorAttr::White, false);
        } else if (line.startsWith("QWARN")) {
            printWithColor(line, TestOutputColorAttr::Yellow, false);
        } else if (line.startsWith("QDEBUG")) {
            printWithColor(line, TestOutputColorAttr::Blue, false);
        } else {
            printWithColor(line, TestOutputColorAttr::Unset, false);
        }
    }
    printWithColor(QString(), TestOutputColorAttr::Unset);
    file.close();
}

//linux console color attribute
enum class StdColorAttr {
    LR = 31,
    LG = 32,
    LY = 33,
    LB = 34,
    LP = 35,
    LC = 36,
    LW = 37,

    //background
    LBK = 10, // x += LBK

    //lighter
    LL = 60, // x += LL

    //underline
    LU = 4,

    //blink
    LBlink = 5,
};

#define OUTPUT_TARGET_STD
#define OUTPUT_TARGET_STD_SYSTEM_CODE
//#define OUTPUT_TARGET_CONSOLE
void TestUtils::printWithColor(const QString &data, TestOutputColorAttr colorStyle, bool end) {

    const auto& wrapLog = [&] {
        if (colorStyle == TestOutputColorAttr::Unset) {
            return data;
        }
        QString styled = "\033[";
        int v = 30 + (int)colorStyle + (int)StdColorAttr::LL;
        styled += QString::number(v) + "m";
        return styled + data + "\033[0m";
    };

    auto wrapped = wrapLog();
#ifdef OUTPUT_TARGET_STD
#ifdef OUTPUT_TARGET_STD_SYSTEM_CODE
    std::cout << wrapped.toLocal8Bit().data();
#else
    std::cout << wrapped.toStdString();
#endif
    if (end) {
        std::cout << std::endl;
    }
#elif defined OUTPUT_TARGET_CONSOLE
#ifdef Q_OS_WIN
    OutputDebugString(reinterpret_cast<const wchar_t*>(wrapped.utf16()));
    if (end) {
        OutputDebugString(L"\n");
    }
#endif
#endif
}
