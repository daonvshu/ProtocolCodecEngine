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
|S|size/所选字段的字节数，默认只计算内容| 2字节              |
|T|type/数据类型标志，必须显式定义| 1字节              |
|C|content/内容，不隐式包含T| C为变长，C8为固定8字节 |
|V|verify/校验类型| CRC16、SUM8、SUM16 |
|E|end/帧尾| FE               |

使用`setLogging`函数设置日志筛选器进行打印，检查解码过程中的错误：

```cpp
Q_LOGGING_CATEGORY(printTest, "print_test")

codecEngine.setLogging(printTest);
```

解析错误可以通过枚举按位组合选择。例如，只记录帧尾错误和 CRC/校验和错误：

```cpp
codecEngine.setDecodeErrorLogFlags(DecodeError::TailMismatch | DecodeError::ChecksumMismatch);
// 全部错误：DecodeError::All；关闭全部解析错误日志：DecodeError::None
```

| 标志 | 记录内容 |
| --- | --- |
| `HeaderMismatch` | 找不到帧头而丢弃的连续字节（合并打印，不逐字节刷屏） |
| `AddressMismatch` | 地址不匹配，包含期望值和收到值 |
| `InvalidLength` | 长度超限、负内容长度或长度溢出 |
| `TailMismatch` | 帧尾不匹配，包含期望字节和收到字节 |
| `ChecksumMismatch` | CRC/校验和错误，包含算法、实际计算值和收到值 |
| `UnknownType` | 未注册解码回调，或按类型确定内容长度时找不到该类型 |
| `Timeout` | 下次收到数据时检测到接收间隔超时，丢弃旧缓存 |
| `BufferOverflow` | 超过缓存限制而丢弃最早的缓存字节 |

默认启用 `AddressMismatch`、`InvalidLength`、`ChecksumMismatch`、`UnknownType` 和 `Timeout`。
错误日志通过 `setLogging` 指定的分类以 warning 输出，不受该分类的 info 开关影响；未设置分类时，
仅保留未注册类型的默认 warning。标志只控制解析错误，不控制正常编码日志。

每条错误日志包含原因、字节数和**完整十六进制字节，不截断**。CRC、帧尾、地址等错误在整帧收齐后
按协议声明长度打印当前候选帧，使用 `boundary=complete-frame` 标识；不会把紧随其后的正常帧一起打印。
长度损坏时无法可靠确定原始帧边界，使用 `boundary=unknown`，打印当前候选帧头至下一帧头或
缓存末尾的可用字节；帧头错误、超时和缓存超限则打印实际丢弃的字节。
正常分包不打印错误，等待后续数据；拒绝坏帧后继续搜索帧头，以便恢复解析后续正常帧。
同一候选帧先检查帧尾，再检查 CRC，记录首先失败的检查。

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
        codecEngine.frameDeclare("H(5AFF)T1S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("STC");
        //注册类型
        codecEngine.registerType<JsonCodec<DataType1>>(this, &TestClass::dataType1Callback);
        //仅编码时，改用下面的注册方式，不要与上一行同时注册
        //codecEngine.registerType<DataType1, JsonCodec>();
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
        codecEngine.frameDeclare("H(5AFF)T1S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("STC");
        //注册类型
        codecEngine.registerType<SerializeCodec<DataType2>>(this, &TestClass::dataType2Callback);
        //仅编码时，改用下面的注册方式，不要与上一行同时注册
        //codecEngine.registerType<DataType2, SerializeCodec>();
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
        codecEngine.frameDeclare("H(5AFF)T1S2CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("STC");
        //注册类型
        codecEngine.registerType<BytesCodec<DataType3>>(this, &TestClass::dataType3Callback);
        //仅编码时，改用下面的注册方式，不要与上一行同时注册
        //codecEngine.registerType<DataType3, BytesCodec>();
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
        codecEngine.frameDeclare("H(5AFF)T1V(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("T");
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
        codecEngine.frameDeclare("H(5AFF)T1S4CV(CRC16)E(FE)");
        //定义校验使用的标志符
        codecEngine.setVerifyFlags("STC");
        //注册类型
        codecEngine.registerType<CompressCodec<DataType4, BytesCodec>>(this, &TestClass::dataType4Callback);
        //仅编码时，改用下面的注册方式，不要与上一行同时注册
        //codecEngine.registerCompressEncoder<DataType4, BytesCodec>();
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

### 8. 协议字段缺省与换序

定义字符串决定线上字节顺序，解码器内部的检查优先级不会改变发送顺序。下面说明的是当前实现的能力，
不是任意字段排列都能工作的保证。每种字段应只定义一次，使用表中大写标志，字符串内不要加入空格。

#### 可以省略哪些字段

| 字段 | 缺省规则 |
| --- | --- |
| T | 必须显式定义，例如 T1；缺少时会调用 `qFatal`，不会从 C 中自动提取类型 |
| H | 可以省略；只有 H 位于首字段时才启用帧头搜索。无帧头时仍会逐位置尝试解析，抗干扰能力取决于其他约束 |
| A | 可以省略；存在时使用 `setAddressValue()` 指定需要匹配的地址 |
| S | 可以省略，但 C 必须固定长度，或使用前置 T 对应的内容长度表 |
| C | 可以省略，适用于没有参数的纯命令，例如 `H(5AFF)T1V(CRC16)E(FE)`，校验范围设置为 T |
| V | 可以省略，此时没有 CRC/校验和保护 |
| E | 可以省略；帧尾不是变长内容的分隔符，不能替代长度定义 |

省略 S 的两种内容定义方式：

```cpp
// 固定八字节内容：调用方编码时也必须传入恰好八字节
codecEngine.frameDeclare("H(5AFF)T1C8V(CRC16)E(FE)");
codecEngine.setVerifyFlags("TC");
```

```cpp
// 不同类型分别确定内容长度；0x03 是没有内容的命令
codecEngine.frameDeclare("H(5AFF)T1CV(CRC16)E(FE)");
codecEngine.setVerifyFlags("TC");
codecEngine.setContentSizeDependsOnType({{0x01, 8}, {0x02, 16}, {0x03, 0}});
// 此方式不要调用 setSizeFlags("C") 或 setSizeFlags("TC")，否则 C 会转为依赖 S。
// 注册的内容编解码器也必须与各类型的长度一致。
```

存在 S 和变长 C 时，C 默认按前置 S 计算长度；存在 V 和 C 时，C 默认参与校验。
`setSizeFlags()`、`setVerifyFlags()` 会追加选中的字段，不会清除已经选中的字段。
S 自身不计入长度。例如 `setSizeFlags("TC")` 且 T1 时，`Size = 1 + 内容字节数`，
所以空内容应写 Size=1；默认只计算 C 时，空内容可以写 Size=0。

#### 哪些字段可以换位

| 换序方式 | 示例 | 依赖条件 |
| --- | --- | --- |
| A、T、S 在内容前交换 | `H(5AFF)A1S2T1CV(CRC16)E(FE)` | S 必须在依赖它的 C 前；校验范围按实际字段设置 |
| T 或 A 放在内容后 | `H(5AFF)S2CT1A1V(CRC16)E(FE)` | 此例的 S 在 C 前，内容长度不依赖后面的 T |
| 固定内容在类型前 | `H(5AFF)C8T1V(CRC16)E(FE)` | C8 不依赖 T 或 S；编码内容必须恰好八字节 |
| 帧尾放到校验前 | `H(5AFF)T1S2CE(FE)V(CRC16)` | V 后计算校验，可以通过 `setVerifyFlags("STCE")` 把 E 纳入校验 |
| H 放在内部 | `T1C8H(5AFF)V(CRC16)E(FE)` | 走一般位置扫描而非首部帧头搜索；建议优先使用首字段 H |

采用类型长度表时，T 必须在 C 前；采用 S 时，S 必须在 C 前。
校验 V 必须位于所有参与校验的字段之后，因为编码器走到 V 时只拥有已经生成的字节。
数值默认小端，可分别用 `T2!`、`A2!`、`S2!`、`V!(CRC16)` 指定大端。

#### 尚不能保证支持的组合

- 没有 S、固定长度和类型长度表，仅靠搜索 E 截取内容。
- 变长 C 在其长度来源 S/T 前，或 V 在参与校验的内容前。
- 重复定义同一种字段，或随意扩大 T/A/S 的字节宽度；当前没有完整的定义合法性检查。
- 固定内容 C8 与 S 混用：当前编码长度计算可能重复计入固定内容长度，建议使用固定长度或变长内容中的一种。
- 固定内容编码时传入长度不符的数据：编码器不会自动补齐、裁剪或拒绝。

## 导入库

```cmake
find_package(ProtocolCodecEngine 1.x.x REQUIRED)

target_link_libraries(${PROJECT_NAME} ProtocolCodec::ProtocolCodecEngine)
```

## 解码优化

下面提出的几个解码优化方案并非强制性的，在高速数据传输的场景中，能够提升解码性能。

### 1. 设置帧尾

解码器先读取帧头、地址、类型和长度等基础字段，确定整帧长度并等待收齐后，先检查帧尾，再检查校验和提取内容。
设置帧尾可以在计算 CRC 前排除帧尾不匹配的候选帧；帧尾本身不用于推导未知的内容长度。

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
