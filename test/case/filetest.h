#pragma once

#include <qobject.h>
#include "protocolcodecengine.h"

PROTOCOL_CODEC_USING_NAMESPACE

struct AdcData {
    char adc[12];
    uint32_t turbine;
};

struct FrameData {
    enum {
        Type = 0
    };

    AdcData data[2048];

    QByteArray toBytes() const {
        return QByteArray((char*)data, 2048 * sizeof(AdcData));
    }

    static FrameData fromBytes(const QByteArray& data) {
        FrameData res{};
        memcpy(&res, data.data(), data.size());
        return res;
    }
};

class FileTest : public QObject {
    Q_OBJECT

public:
    explicit FileTest(QObject* parent = nullptr);

private slots:
    void initTestCase();

    void decodeFromFileTest();

    void cleanup();
    void cleanupTestCase();

private:
    void callbackFunc(const FrameData& data);

private:
    ProtocolCodecEngine codec;
    int objSize = 0;
};

