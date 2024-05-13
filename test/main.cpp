#include <QCoreApplication>

#include "protocolcodecengine.h"

#include <qjsondocument.h>
#include <qjsonobject.h>

PROTOCOL_CODEC_USING_NAMESPACE

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

    static DataType2 fromJson(const QByteArray& content) {
        DataType2 data;
        QJsonDocument doc = QJsonDocument::fromJson(content);
        if (!doc.isNull()) {
            data.data = doc.object().value("d").toString();
        }
        return data;
    }

    QByteArray toJson() const {
        QJsonObject obj;
        obj.insert("d", data);
        return QJsonDocument(obj).toJson();
    }
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

class TestClass {
public:
    void test() {
        codecEngine.frameDeclare("H(5AFF)S2CV(CRC16)E(FE)");
        codecEngine.setVerifyFlags("SC");
        codecEngine.setTypeEncodeByteSize(3);
        codecEngine.registerType<JsonCodec<DataType1>>(this, &TestClass::dataType1Callback);
        codecEngine.registerType<SerializeCodec<DataType2>>(this, &TestClass::dataType2Callback);
        codecEngine.registerType<BytesCodec<DataType3>>(this, &TestClass::dataType3Callback);
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

private:
    ProtocolCodecEngine codecEngine;
};

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    TestClass testClass;
    testClass.test();

    return a.exec();
}