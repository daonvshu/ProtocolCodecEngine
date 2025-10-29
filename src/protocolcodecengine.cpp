#include "protocolcodecengine.h"

#include "flagdata/datacontent.h"
#include "utils/protocoltemplatedecoder.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolCodecEngine::ProtocolCodecEngine(QObject *parent)
    : QObject(parent)
{
}

ProtocolCodecEngine::~ProtocolCodecEngine() {
    clearFlags(flags);
}

void ProtocolCodecEngine::frameDeclare(const QString &templateStr) {
    clearFlags(flags);
    flags = ProtocolTemplateDecoder::parse(templateStr);
    decoder.setFlags(flags);
    encoder.setFlags(flags);

    //设置size计算和验证的默认flag
    QMap<int, ProtocolFlagData*> flagSet;
    auto next = flags;
    while (next != nullptr) {
        flagSet.insert((int)next->flag, next);
        next = next->next;
    }
    if (flagSet.contains((int)ProtocolFlag::Flag_Content)) {
        auto* flagData = flagSet.value((int)ProtocolFlag::Flag_Content);
        if (flagSet.contains((int)ProtocolFlag::Flag_Size)) {
            flagData->isSizeTarget = true;
        }
        if (flagSet.contains((int)ProtocolFlag::Flag_Verify)) {
            flagData->isVerifyTarget = true;
        }
    }
}

void ProtocolCodecEngine::setVerifyFlags(const QString &flagStr) const {
    auto verifyFlags = ProtocolTemplateDecoder::parse(flagStr, false);

    auto targets = getFlagDataByFlags(verifyFlags);
    for (auto target : targets) {
        target->isVerifyTarget = true;
    }
    clearFlags(verifyFlags);
}

void ProtocolCodecEngine::setSizeFlags(const QString& flagStr) const {
    auto verifyFlags = ProtocolTemplateDecoder::parse(flagStr, false);

    auto targets = getFlagDataByFlags(verifyFlags);
    for (auto target : targets) {
        target->isSizeTarget = true;
    }
    clearFlags(verifyFlags);
}

void ProtocolCodecEngine::setContentSizeDependsOnType(const QMap<int, int>& contentSizeFromType) const {
    auto next = flags;
    while (next != nullptr) {
        if (next->flag == ProtocolFlag::Flag_Content) {
            dynamic_cast<ProtocolFlagDataContent*>(next)->setContentSizeFromType(contentSizeFromType);
            break;
        }
    }
}

void ProtocolCodecEngine::setAddressValue(uint32_t address) const {
    decoder.setAddress(address);
    encoder.setAddress(address);
}

void ProtocolCodecEngine::setSizeMaxValue(int value) const {
    decoder.setSizeMaxValue(value);
}

void ProtocolCodecEngine::setBufferMaxSize(int size) {
    decoder.setBufferMaxSize(size);
}

void ProtocolCodecEngine::setDecodeTimeout(int ms) {
    decoder.setDecodeTimeout(ms);
}

void ProtocolCodecEngine::setLogging(LoggingCategoryPtr categoryPtr) {
    decoder.setLogging(categoryPtr);
    encoder.setLogging(categoryPtr);
}

void ProtocolCodecEngine::appendBuffer(const QByteArray &buffer) {
    decoder.addBuffer(buffer);
}

void ProtocolCodecEngine::clearDecodeCacheBuffer() {
    decoder.clearCache();
}

void ProtocolCodecEngine::enableRecordDecodeMetaData() {
    decoder.enableMetaDataRecord();
}

ProtocolMetaData ProtocolCodecEngine::getLastEncodeMetaData() const {
    return encoder.getLastMetaData();
}

ProtocolMetaData ProtocolCodecEngine::getLastEncodeMetaData(int type) const {
    return encoder.getLastMetaData(type);
}

ProtocolMetaData ProtocolCodecEngine::getLastDecodeMetaData() const {
    return decoder.getLastMetaData();
}

ProtocolMetaData ProtocolCodecEngine::getLastDecodeMetaData(int type) const {
    return decoder.getLastMetaData(type);
}

void ProtocolCodecEngine::clearFlags(ProtocolFlagData* flags) {
    auto next = flags;
    while (next != nullptr) {
        auto nextPtr = next->next;
        delete next;
        next = nextPtr;
    }
}

QList<ProtocolFlagData*> ProtocolCodecEngine::getFlagDataByFlags(ProtocolFlagData* makeFlags) const {
    QSet<int> flagSet;
    auto next = makeFlags;
    while (next != nullptr) {
        flagSet.insert((int)next->flag);
        next = next->next;
    }

    QList<ProtocolFlagData*> targets;
    next = flags;
    while (next != nullptr) {
        if (flagSet.contains((int)next->flag)) {
            targets << next;
        }
        next = next->next;
    }
    return targets;
}

PROTOCOL_CODEC_NAMESPACE_END
