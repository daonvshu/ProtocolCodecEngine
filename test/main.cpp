#include <iostream>
#include <ostream>
#include <QCoreApplication>

#include "protocolcodecengine.h"

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qfile.h>
#include <qelapsedtimer.h>
#include <qlogcollector.h>

PROTOCOL_CODEC_USING_NAMESPACE

Q_LOGGING_CATEGORY(printTest, "print_test")

struct DataType1 {
    enum {
        Type = 1
    };

    int data = -1;

    static DataType1 fromJson(const QJsonDocument& doc) {
        DataType1 data;
        data.data = doc.object().value("d").toInt();
        return data;
    }

    QJsonDocument toJson() const {
        QJsonObject obj;
        obj.insert("d", data);
        return QJsonDocument(obj);
    }
};

struct DataType2 {
    enum {
        Type = 2
    };

    QString data;
};

inline QDataStream& operator<<(QDataStream& out, const DataType2& data) {
    out << data.data;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, DataType2& data) {
    in >> data.data;
    return in;
}

struct DataType3 {
    enum {
        Type = 3
    };

    int data = 0;

    static DataType3 fromBytes(const QByteArray& content) {
        DataType3 data;
        memcpy(&data.data, content.data(), 4);
        return data;
    }

    QByteArray toBytes() const {
        char bytes[4];
        memcpy(bytes, (void*)&data, 4);
        return QByteArray(bytes, 4);
    }
};

struct DataType4 {
    enum {
        Type = 4
    };

    QByteArray data;

    static DataType4 fromBytes(const QByteArray& content) {
        DataType4 data;
        data.data = content;
        return data;
    }

    QByteArray toBytes() const {
        return data;
    }
};


class TestClass {
public:
    void test() {
        codecEngine.frameDeclare("H(5AFF)S4CE(FE)");
        codecEngine.setVerifyFlags("SC");
        codecEngine.setTypeEncodeByteSize(3);
        codecEngine.registerType<JsonCodec<DataType1>>(this, &TestClass::dataType1Callback);
        codecEngine.registerType<SerializeCodec<DataType2>>(this, &TestClass::dataType2Callback);
        codecEngine.registerType<BytesCodec<DataType3>>(this, &TestClass::dataType3Callback);
        codecEngine.registerType<CompressCodec<DataType4, BytesCodec, 3>>(this, &TestClass::dataType4Callback);
        codecEngine.registerType<0x13>(this, &TestClass::dataType13Callback);

        DataType1 data1{ 10 };
        auto bytes = codecEngine.encode(data1);
        bytes.prepend("saifasd").append("saoifia");
        codecEngine.appendBuffer(bytes);

        DataType2 data2{ "abcde" };
        bytes = codecEngine.encode(data2);
        bytes.prepend("sikf8").append("iasf");
        codecEngine.appendBuffer(bytes);

        DataType3 data3{ 20 };
        bytes = codecEngine.encode(data3);
        bytes.prepend("asfas").append("gdser3");
        codecEngine.appendBuffer(bytes);

        bytes = codecEngine.encode<0x13>();
        bytes.prepend("fasad").append("fgrs4");
        codecEngine.appendBuffer(bytes);

        bytes = codecEngine.encode<0x14>();
        codecEngine.appendBuffer(bytes);

        DataType4 data4;
        QFile file("protocol_codec_engine_test.pdb");
        file.open(QIODevice::ReadOnly);
        data4.data = file.readAll();
        qDebug() << "data4 source file size:" << data4.data.size();
        file.close();
        bytes = codecEngine.encode(data4);
        qDebug() << "data4 encode data size:" << bytes.size();
        bytes.prepend("fawew").append("assfs");
        codecEngine.appendBuffer(bytes);
    }

    void dataType1Callback(const DataType1& data) {
        qDebug() << "data1 callback:" << data.data;
    }

    void dataType2Callback(const DataType2& data) {
        qDebug() << "data2 callback:" << data.data;
    }

    void dataType3Callback(const DataType3& data) {
        qDebug() << "data3 callback:" << data.data;
    }

    void dataType13Callback() {
        qDebug() << "data13 callback!";
    }

    void dataType4Callback(const DataType4& data) {
        qDebug() << "data4 compress callback size:" << data.data.size();
    }

private:
    ProtocolCodecEngine codecEngine;
};

struct AdcData {
    char adc[12];
    uint32_t turbine;

    AdcData() {
        char test = '1';
        memcpy(adc, &test, 12);
        turbine = 20;
    }
    QList<uint32_t> showData() const {
        QList<uint32_t> res;
        for(int i = 0; i < 4; i++) {
            uint32_t data = 0u;
            memcpy(&data, adc + i * 3, 3);
            res.append(data);
        }
        return res;
    }

    QString toHex() const {
        QByteArray res;
        res.append(adc, 12);
        res.append(QByteArray(reinterpret_cast<const char*>(&turbine), sizeof(turbine)));
        return res.toHex(' ');
    }
    //
    //    AdcData& swapBytes() {
    //        auto* dataPtr = reinterpret_cast<const char*>(this);
    //        QByteArray rawData(dataPtr, sizeof(AdcData));
    //        for(auto iter = rawData.begin(); iter != rawData.end(); iter += 4) {
    //            std::reverse(iter, iter + 4);
    //        }
    //        return *this;
    //    }
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

    void toHex() const {
        for(int i = 0; i< 2048; i++) {
            qDebug() << i << " " << data[i].toHex();
        }
    }

    //    FrameData& swapAdcData() {
    //        for(auto& d : data) {
    //            d.swapBytes();
    //        }
    //        return *this;
    //    }
};

class TestClass2 {
public:
    void test() {
        codecEngine.frameDeclare("H(5AFF)S2CV(SUM16)E(FE)");
        codecEngine.setVerifyFlags("SC");
        codecEngine.registerType<BytesCodec<FrameData>>(this, &TestClass2::callbackFunc);
        codecEngine.setBufferMaxSize(sizeof(FrameData) + 9);
        codecEngine.setSizeMaxValue(sizeof(FrameData) + 2);
        //codecEngine.setDecodeTimeout(10);
        codecEngine.setLogging(printTest);

        QFile file("1.bin");
        file.open(QIODevice::ReadOnly);

        QElapsedTimer timer;
        timer.start();
        while (!file.atEnd()) {
            auto data = file.read(102400);
            codecEngine.appendBuffer(data);
        }
        std::cout << "decode used time:" << timer.elapsed() << " decoded object size:" << objSize << std::endl;
        file.close();
    }

    void callbackFunc(const FrameData& data) {
        objSize++;
    }

private:
    ProtocolCodecEngine codecEngine;
    int objSize = 0;
};

struct MicroFrame {
    float pdIn[127];
    float lpf[127];
    float tmp[1];
    float pzt[1];
};

struct  FD{
    enum {
        Type = 22
    };
    MicroFrame data[8];
    QByteArray toBytes() const {
        QByteArray tmp(sizeof(FD), 0);
        memcpy(tmp.data(), this, sizeof(FD));
        return tmp;
    }
    static FD fromBytes(const QByteArray& tmp) {
        FD res;
        memcpy(&res, tmp.data(), tmp.size());
        return res;
    }
};
Q_DECLARE_METATYPE(FD)

class TestClass3 {
public:
    void test() {
        codecEngine.frameDeclare("H(FEFE)S2CV(CRC16)E(FEFF)");
        codecEngine.setVerifyFlags("SC");
        codecEngine.registerType<FD, BytesCodec>();

        FD data;
        for (int j = 0; j < 8; j++) {
            for(int i = 0; i < 127; i++) {
                data.data[j].lpf[i] = 2;
                data.data[j].pdIn[i] = 4;
            }
            data.data[j].tmp[0] = 0;
            data.data[j].pzt[0] = 0;
        }

        auto encoded = codecEngine.encode(data);
        qDebug() << encoded.toHex(' ');
    }

private:
    ProtocolCodecEngine codecEngine;
};

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    logcollector::styleConfig
            .consoleApp()
            .ide_clion(false)
            .wordWrap(360)
            .projectSourceCodeRootPath(ROOT_PROJECT_PATH)
            ;
    logcollector::QLogCollector::instance().registerLog();

    TestClass2 testClass;
    testClass.test();

    return 0;
}