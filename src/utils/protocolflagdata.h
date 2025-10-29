#pragma once

#include "../protocolflags.h"
#include "../protocolmetadata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagData {
public:
    explicit ProtocolFlagData(ProtocolFlag flag, int byteSize);
    virtual ~ProtocolFlagData() = default;

    virtual QString dataToString();

    virtual bool verify(char* data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) = 0;

    void bindPrev(ProtocolFlagData* prevPtr);

    virtual QByteArray getBytesContent() const = 0;

protected:
    int getCurrentByteOffset() const;

public:
    ProtocolFlag flag;
    int byteSize;
    ProtocolFlagData* prev = nullptr;
    ProtocolFlagData* next = nullptr;
    bool isVerifyTarget = false;
    bool isSizeTarget = false;
    bool isLittleEndian = true;
};

inline QDebug operator<<(QDebug debug, ProtocolFlagData* data) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "[";
    debug << data->flag;
    debug << "]";
    auto str = data->dataToString();
    if (!str.isEmpty()) {
        debug << ": ";
        debug << str;
    }
    return debug;
}

PROTOCOL_CODEC_NAMESPACE_END