#pragma once

#include "../protocolflags.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolFlagData {
public:
    virtual ~ProtocolFlagData() = default;
    explicit ProtocolFlagData(ProtocolFlag flag);

    virtual QString dataToString();

    virtual bool verify(char* data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) = 0;

    virtual void doFrameOffset(int& offset) = 0;

    virtual QSharedPointer<ProtocolFlagData> copy() const = 0;

public:
    ProtocolFlag flag;
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