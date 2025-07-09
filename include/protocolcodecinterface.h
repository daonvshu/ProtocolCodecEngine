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

    virtual void setFlags(const QList<QSharedPointer<ProtocolFlagData>>& flags);

    void setVerifyFlags(const QList<QSharedPointer<ProtocolFlagData>>& flags);

    void setTypeByteSize(int size);

    void setLogging(LoggingCategoryPtr categoryPtr);

protected:
    QSharedPointer<ProtocolFlagData> enumFlags[(int)ProtocolFlag::Flag_Max];
    QList<QSharedPointer<ProtocolFlagData>> protocolFlags;

    LoggingCategoryPtr debugPtr = nullptr;

    int mTypeByteSize;

protected:
    template<typename T>
    QSharedPointer<T> get(ProtocolFlag flag) {
        return qSharedPointerCast<T>(enumFlags[(int)flag]);
    }

    void printInfo(const std::function<QString()>& getMessage) const;

    void printWarning(const std::function<QString()>& getMessage) const;
};

PROTOCOL_CODEC_NAMESPACE_END