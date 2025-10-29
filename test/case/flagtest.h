#pragma once

#include <qobject.h>

class FlagTest : public QObject {
    Q_OBJECT

public:
    explicit FlagTest(QObject* parent = nullptr);

private slots:
    void initTestCase();

    void cleanup();
    void cleanupTestCase();
};