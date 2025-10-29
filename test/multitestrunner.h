#pragma once

#include <QtTest/QTest>

#include "testutils.h"

template<typename... Arg> struct MultiTestRunner;
template<typename T, typename... Arg>
struct MultiTestRunner<T, Arg...> : MultiTestRunner<Arg...> {

    template<typename... Params>
    static int run(const QString& target, Params... params) {
        T t(params...);
        int result = QTest::qExec(&t, QStringList() << target << "-o" << "test.txt");
        TestUtils::printTestLog("test.txt");

        result += MultiTestRunner<Arg...>::run(target, params...);
        return result;
    }
};

template<> struct MultiTestRunner<> {

    template<typename... Params>
    static int run(const QString&, Params...) { return 0; }
};