# ProtocolCodec

用于帧协议编解码工具

## 使用方法

### 1. 定义协议格式

```cpp
ProtocolCodecEngine codecEngine;
//定义协议帧格式
codecEngine.frameDeclare("H(5AFF)S2CV(CRC16)E(FE)");
//定义校验使用的标志符
codecEngine.setVerifyFlags("SC");
```
|标志符|说明|示例|
|:--:|:--|:--|
|H|header/帧头|5AFF|
|S|size/内容字节数大小|2|
|C|content/内容（隐式包含数据类型）||
|V|verify/校验类型|CRC16、SUM8、SUM16|
|E|end/帧尾|FE|

### 2. 注册数据类型

目前有两种内容编码方式，分别对应`JsonCodec`和`SerializeCodec`:  

- json编码器：`JsonCodec<T>`

```cpp
struct DataType1 {
    enum {
        Type = 1 //必须设置，用于区分数据类型
    };

    int data = -1;

    static DataType1 fromJson(const QByteArray& content) {
        DataType1 data;
        QJsonDocument doc = QJsonDocument::fromJson(content);
        if (!doc.isNull()) {
            data.data = doc.object().value("d").toInt();
        }
        return data;
    }

    QByteArray toJson() const {
        QJsonObject obj;
        obj.insert("d", data);
        return QJsonDocument(obj).toJson();
    }
};

//注册DataType1
class TestClass {
public:
    void registerType() {
        //定义协议帧格式
        codecEngine.frameDeclare("H(5AFF)S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("SC");
        //注册类型
        codecEngine.registerType<JsonCodec<DataType1>>(this, &TestClass::dataType1Callback);
        //不使用解码器
        codecEngine.registerType<DataType1, JsonCodec>();
    }

    void dataType1Callback(const DataType1& data) {
        qDebug() << "data1 callback:" << data.data;
    }

private:
    ProtocolCodecEngine codecEngine;
}
```

- 序列化编码器：`SerializeCodec<T>`

```cpp
struct DataType2 {
    enum {
        Type = 2 //必须设置，用于区分数据类型
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

//注册DataType2
class TestClass {
public:
    void registerType() {
        //定义协议帧格式
        codecEngine.frameDeclare("H(5AFF)S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("SC");
        //注册类型
        codecEngine.registerType<SerializeCodec<DataType2>>(this, &TestClass::dataType2Callback);
        //不使用解码器
        codecEngine.registerType<DataType2, SerializeCodec>();
    }

    void dataType2Callback(const DataType2& data) {
        qDebug() << "data2 callback:" << data.data;
    }

private:
    ProtocolCodecEngine codecEngine;
}
```

### 3. 数据解析

将QByteArray数据传递给`ProtocolCodecEngine`即可，会自动回调到`registerType`时的回调函数。

```cpp
codecEngine.appendBuffer(bytes);
```

### 4. 数据编码

编码返回完整帧数据，传递对象即可：

```cpp
DataType1 data1{ 10 };
auto bytes = codecEngine.encode(data1);

DataType2 data2{ "abcde" };
bytes = codecEngine.encode(data2);
```

### 5. 空消息内容编解码

对于没有消息内容的指令编解码，使用其`registerType`重载函数：

```cpp
class TestClass {
public:
    void registerType() {
        //定义协议帧格式
        codecEngine.frameDeclare("H(5AFF)S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("SC");
        //注册类型
        codecEngine.registerType<0x03>(this, &TestClass::dataType3Callback);
        //编码
        auto bytes = codecEngine.encode<0x03>();
    }

    void dataType3Callback() {
        qDebug() << "data3 callback!";
    }

private:
    ProtocolCodecEngine codecEngine;
}
```