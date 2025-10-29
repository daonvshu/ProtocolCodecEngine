#pragma once

#include <qobject.h>
#include "protocolcodecengine.h"

PROTOCOL_CODEC_USING_NAMESPACE

struct FunctionalTestData {
    enum {
        Type = 100
    };

    int data1 = 0;
    int data2 = 0;

    static FunctionalTestData fromBytes(const QByteArray& content) {
        FunctionalTestData data;
        memcpy(&data, content.data(), sizeof(FunctionalTestData));
        return data;
    }

    QByteArray toBytes() const {
        enum {
            Size = sizeof(FunctionalTestData)
        };
        char bytes[Size];
        memcpy(bytes, this, Size);
        return QByteArray(bytes, Size);
    }
};

class FunctionalTest : public QObject {
    Q_OBJECT

public:
    explicit FunctionalTest(QObject* parent = nullptr);

private slots:
    void initTestCase();

    void encodeTest();
    void decodeTest();
    void garbageDecodeTest();
    void frameBreakTest();

    void cleanup();
    void cleanupTestCase();

private:
    void callback(const FunctionalTestData& data);

private:
    ProtocolCodecEngine codec;
    QList<FunctionalTestData> lastDecodedData;
};