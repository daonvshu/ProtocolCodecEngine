#include "protocolcodecinterface.h"

#include "flagdata/dataheader.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "flagdata/dataend.h"

#include <qdebug.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolCodecInterface::ProtocolCodecInterface(QObject *parent)
    : QObject(parent)
    , mTypeByteSize(2)
{
}

void ProtocolCodecInterface::setFlags(const QList<QSharedPointer<ProtocolFlagData>> &flags) {
    for (const auto& flag : flags) {
        protocolFlags.append(flag->copy());
    }

    for (const auto& flag : protocolFlags) {
        enumFlags[(int)flag->flag] = flag;
    }

    auto flagEnd = get<ProtocolFlagDataEnd>(ProtocolFlag::Flag_End);
    if (flagEnd) {
        flagEnd->setDependentFlag(get<ProtocolFlagDataSize>(ProtocolFlag::Flag_Size),
                                  get<ProtocolFlagDataContent>(ProtocolFlag::Flag_Content),
                                  get<ProtocolFlagDataVerify>(ProtocolFlag::Flag_Verify));
    }

    auto flagContent = get<ProtocolFlagDataContent>(ProtocolFlag::Flag_Content);
    if (flagContent) {
        flagContent->setDependentFlag(get<ProtocolFlagDataSize>(ProtocolFlag::Flag_Size),
                                      get<ProtocolFlagDataVerify>(ProtocolFlag::Flag_Verify),
                                      get<ProtocolFlagDataEnd>(ProtocolFlag::Flag_End));
    }
}

void ProtocolCodecInterface::setVerifyFlags(const QList<QSharedPointer<ProtocolFlagData>> &flags) {
    auto verifyFlag = get<ProtocolFlagDataVerify>(ProtocolFlag::Flag_Verify);
    if (verifyFlag) {
        QList<QSharedPointer<ProtocolFlagData>> targetFlags;
        for (const auto& flag : flags) {
            targetFlags.append(enumFlags[(int)flag->flag]);
        }
        verifyFlag->setVerifyFlags(targetFlags);
    }
}

void ProtocolCodecInterface::setTypeByteSize(int size) {
    mTypeByteSize = size;
}

void ProtocolCodecInterface::setLogging(LoggingCategoryPtr categoryPtr) {
    debugPtr = categoryPtr;
}

void ProtocolCodecInterface::printInfo(const std::function<QString()>& getMessage) const {
    if (debugPtr) {
        qCInfo(debugPtr) << getMessage();
    }
}

void ProtocolCodecInterface::printWarning(const std::function<QString()>& getMessage) const {
    if (debugPtr) {
        qCWarning(debugPtr) << getMessage();
    }
}

PROTOCOL_CODEC_NAMESPACE_END
