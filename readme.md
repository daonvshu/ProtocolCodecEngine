# ProtocolCodec

用于帧协议编解码工具

## 使用方法

### 1. 定义协议格式

```cpp
ProtocolCodecEngine codecEngine;
//定义协议帧格式
codecEngine.frameDeclare("H(5AFF)A1S2T1CV(CRC16)E(FE)");
//定义校验使用的标志符，默认为"C"
codecEngine.setVerifyFlags("STC");
//定义计算字节数使用的标志符，默认为"C"
codecEngine.setSizeFlags("TC");
//设置地址值
codecEngine.setAddressValue(0x01);
```
|标志符|说明| 示例               |
|:--:|:--|:-----------------|
|H|header/帧头| 5AFF             |
|A|address/地址| 1字节              |
|S|size/内容字节数大小| 2字节              |
|T|type/数据类型标志| 1字节              |
|C|content/内容|                  |
|V|verify/校验类型| CRC16、SUM8、SUM16 |
|E|end/帧尾| FE               |

使用`setLogging`函数设置日志筛选器进行打印，检查解码过程中的错误：

```cpp
Q_LOGGING_CATEGORY(printTest, "print_test")

codecEngine.setLogging(printTest);
```

### 2. 注册数据类型

目前有三种内容编码方式，分别对应`JsonCodec`、`SerializeCodec`、`BytesCodec`:  

- json编码器：`JsonCodec<T>`

```cpp
struct DataType1 {
    enum {
        Type = 1 //必须设置，用于区分数据类型
    };

    int data = -1;

    //json数据解码
    static DataType1 fromJson(const QJsonDocument& doc) {
        DataType1 data;
        data.data = doc.object().value("d").toInt();
        return data;
    }

    //json数据编码
    QJsonDocument toJson() const {
        QJsonObject obj;
        obj.insert("d", data);
        return QJsonDocument(obj);
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

使用[QDataUtil](https://github.com/daonvshu/qjsonutil)能够简化json的转换：

```cpp

template<typename T>
struct DataDumpProtocol : DataDumpInterface {
    //json数据解码
    static T fromJson(const QJsonDocument& doc) {
        T data;
        auto obj = doc.object();
        data.DataDumpInterface::fromJson(obj);
        return data;
    }

    //json数据编码
    QJsonDocument toJson() const {
        return QJsonDocument(this->dumpToJson());
    }
};

struct DataType1 : DataDumpProtocol<DataType1> {
    enum {
        Type = 1 //必须设置，用于区分数据类型
    };

    DATA_KEY(int, data);
    
    QList<DataReadInterface *> prop() override {
        return { &data };
    }
};
```

- 序列化编码器：`SerializeCodec<T>`

```cpp
struct DataType2 {
    enum {
        Type = 2 //必须设置，用于区分数据类型
    };

    QString data;
};

//数据序列化
inline QDataStream& operator<<(QDataStream& out, const DataType2& data) {
    out << data.data;
    return out;
}

//数据反序列化
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

- 字节数组编码器：`BytesCodec`

```cpp
struct DataType3 {
    enum {
        Type = 3 //必须设置，用于区分数据类型
    };

    int data = 0;

    //字节数组转对象成员
    static DataType3 fromBytes(const QByteArray& content) {
        DataType3 data;
        memcpy(&data.data, content.data(), 4);
        return data;
    }

    //复制对象数据到字节数组
    QByteArray toBytes() const {
        char bytes[4];
        memcpy(bytes, (void*)&data, 4);
        return QByteArray(bytes, 4);
    }
};

//注册DataType3
class TestClass {
public:
    void registerType() {
        //定义协议帧格式
        codecEngine.frameDeclare("H(5AFF)S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("SC");
        //注册类型
        codecEngine.registerType<BytesCodec<DataType2>>(this, &TestClass::dataType2Callback);
        //不使用解码器
        codecEngine.registerType<DataType2, BytesCodec>();
    }

    void dataType3Callback(const DataType3& data) {
        qDebug() << "data3 callback:" << data.data;
    }

private:
    ProtocolCodecEngine codecEngine;
}
```

### 3. 数据解析

将QByteArray数据传递给`ProtocolCodecEngine`即可，会自动回调到`registerType`时的回调函数。一般数据来源于串口、socket等等：

```cpp
codecEngine.appendBuffer(socket->readAll());
```

### 4. 数据编码

编码返回完整帧数据，传递对象即可：

```cpp
DataType1 data1{ 10 };
socket->write(codecEngine.encode(data1));

DataType2 data2{ "abcde" };
socket->write(codecEngine.encode(data2));
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

### 6. 数据压缩编码器

如果在可预期的内容传输中字节数过多，可以使用数据压缩编码器`CompressCodec`，该编码器需要配合其他编码器一起使用：

```cpp
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
    void registerType() {
        //定义协议帧格式
        codecEngine.frameDeclare("H(5AFF)S4CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("SC");
        //注册类型
        codecEngine.registerType<CompressCodec<DataType4, BytesCodec>>(this, &TestClass::dataType4Callback);
        //仅编码
        codecEngine.registerCompressEncoder<DataType4, BytesCodec>();
    }

    void dataType4Callback(const DataType4& data) {
        qDebug() << "data4 callback size:" << data.data.size();
    }

private:
    ProtocolCodecEngine codecEngine;
}
```

### 7. 自定义编解码器

继承`ProtocolTypeCodec<T>`编解码基类可以实现自己的编解码逻辑：

```cpp
template<typename T>
class MyCodec : public ProtocolTypeCodec<T> {
public:
    // 解码器按照帧协议格式解码成功会回调到该函数
    T decode(const QByteArray &content) override {
        // decode content bytes to type 'T'
    }
    
    // 编码器编码对象内容时会调用该函数
    QByteArray encode(const T &data) override {
        // encode type 'T' data to bytes
    }
};

```

## 解码优化

下面提出的几个解码优化方案并非强制性的，在高速数据传输的场景中，能够提升解码性能。

### 1. 设置帧尾

解码器按照 `帧头->帧尾->数据长度->校验` 的过程验证是否是有效帧，如果设置了帧尾，能够提前终止验证。

### 2. 设置帧长度最大值

这里帧长度指的是内容长度，即`S`标志符的最大值，如果能预期知道所有帧中最大帧的内容长度+类型字节数，那么设置最大值能够提前终止验证避免校验垃圾数据，造成时间上的浪费。**设置最大值需要额外小心，否则会造成数据丢失**，使用`setSizeMaxValue`函数设置最大值：

```cpp
codecEngine.setSizeMaxValue(sizeof(MyMaxFrameData)+2);
```

### 3. 设置帧缓存最大值

在断帧的情况下，解码器会缓存当前数据，下一段数据到来时重新验证是否存在有效帧。如果能够确定最大帧长度，那么理论上解码窗口不会超过最大帧长度，这样能够提前清理垃圾数据。默认情况下缓存窗口的大小是10M，合理设置缓存大小能够防止垃圾数据过多导致内存爆炸，特别是攻击者有意发送大量垃圾数据的情况。**同样的设置缓存大小需要额外小心，否则会造成丢帧**，使用`setBufferMaxSize`来设置缓存最大值，这里最大值表示编码后的帧最大值，除了最大帧内容长度，还需要包含帧头、大小、校验等等字节数，如果不能明确字节数，可以设置一个固定的预期最大值：

```cpp
codecEngine.setBufferMaxSize(sizeof(MyMaxFrameData)+9);
```

### 4. 设置解码超时时间

发生断帧时，如果对方在超时时间内还没有发送数据，那么解码器认为当前帧已经发送完成，剩余的缓存为垃圾数据，需要清理。**如果使用了超时时间，需要注意由于解码回调函数是同步调用，函数中需要尽快处理数据，发生阻塞时可能会触发超时机制造成丢帧**，使用`setDecodeTimeout`设置解码超时时间(ms):

```cpp
codecEngine.setDecodeTimeout(20);
```

### 5. 使用字节编码器

如果是能否确定大小的基本数据类型，使用字节编码器配合`memcpy`函数能够加快编解码内容的时间，如果帧结构体非常大的情况下，配合压缩编码器能够有效的降低传输的数据量大小。