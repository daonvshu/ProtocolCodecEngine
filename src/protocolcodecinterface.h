#pragma once

#include <qobject.h>
#include <qloggingcategory.h>

#include "global.h"
#include "utils/protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

typedef const QLoggingCategory& (*LoggingCategoryPtr)();

class ProtocolCodecInterface : public QObject {
public:
    explicit ProtocolCodecInterface(QObject *parent = nullptr);

    virtual void setFlags(ProtocolFlagData* flags);

    void setAddress(uint32_t address) const;

    void setLogging(LoggingCategoryPtr categoryPtr);

protected:
    ProtocolFlagData* protocolFlags = nullptr;

    LoggingCategoryPtr debugPtr = nullptr;

protected:
    void printInfo(const std::function<QString()>& getMessage) const;

    void printWarning(const std::function<QString()>& getMessage) const;
};

PROTOCOL_CODEC_NAMESPACE_END