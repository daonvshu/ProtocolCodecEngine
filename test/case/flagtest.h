#pragma once

#include <qobject.h>

#include "functionaltest.h"

class FlagTest : public QObject {
    Q_OBJECT

public:
    explicit FlagTest(QObject* parent = nullptr);

private slots:
    void initTestCase();

    void bigEndianTest();

    void cleanup();
    void cleanupTestCase();

private:
    void callback(const FunctionalTestData& data);

private:
    QList<FunctionalTestData> lastDecodedData;
};