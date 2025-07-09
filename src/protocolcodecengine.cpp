#include "protocolcodecengine.h"
#include "utils/protocoltemplatedecoder.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolCodecEngine::ProtocolCodecEngine(QObject *parent)
    : QObject(parent)
{
}

void ProtocolCodecEngine::frameDeclare(const QString &templateStr) {
    auto flags = ProtocolTemplateDecoder::parse(templateStr);
    decoder.setFlags(flags);
    encoder.setFlags(flags);
}

void ProtocolCodecEngine::setVerifyFlags(const QString &flagStr) {
    auto verifyFlags = ProtocolTemplateDecoder::parse(flagStr, false);
    decoder.setVerifyFlags(verifyFlags);
    encoder.setVerifyFlags(verifyFlags);
}

void ProtocolCodecEngine::setTypeEncodeByteSize(int size) {
    decoder.setTypeByteSize(size);
    encoder.setTypeByteSize(size);
}

void ProtocolCodecEngine::setSizeMaxValue(int value) {
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

PROTOCOL_CODEC_NAMESPACE_END