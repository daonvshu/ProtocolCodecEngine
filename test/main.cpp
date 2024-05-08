#include <QCoreApplication>

#include "protocolcodecengine.h"
#include "utils/protocoltemplatedecoder.h"

PROTOCOL_CODEC_USING_NAMESPACE
int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    auto results = ProtocolTemplateDecoder::parse("H(5AFF)S2CV(CRC)E(FE)");
    for (const auto& r : results) {
        qDebug() << r.data();
    }

    return a.exec();
}