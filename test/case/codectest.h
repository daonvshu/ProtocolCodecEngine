#pragma once

#include <qobject.h>

class CodecTest : public QObject {
    Q_OBJECT

public:
    explicit CodecTest(QObject* parent = nullptr);

private slots:
    void initTestCase();

    void cleanup();
    void cleanupTestCase();
};