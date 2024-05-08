#include "protocolcodecengine.h"
#include "utils/protocoltemplatedecoder.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolCodecEngine::ProtocolCodecEngine(QObject *parent)
    : QObject(parent)
{
}

void ProtocolCodecEngine::frameDeclare(const QString &templateStr) {

}

PROTOCOL_CODEC_NAMESPACE_END