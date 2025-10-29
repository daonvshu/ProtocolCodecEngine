#include "protocolcodecinterface.h"

#include "flagdata/dataaddress.h"

#include <qdebug.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolCodecInterface::ProtocolCodecInterface(QObject *parent)
    : QObject(parent)
{
}

void ProtocolCodecInterface::setFlags(ProtocolFlagData* flags) {
    protocolFlags = flags;
}

void ProtocolCodecInterface::setAddress(uint32_t address) const {
    auto next = protocolFlags;
    while (next != nullptr) {
        if (next->flag == ProtocolFlag::Flag_Address) {
            dynamic_cast<ProtocolFlagDataAddress*>(next)->address = address;
            break;
        }
        next = next->next;
    }
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
