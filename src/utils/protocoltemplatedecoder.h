#pragma once

#include "../protocolflags.h"
#include "../global.h"

#include "protocolflagdata.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

class ProtocolTemplateDecoder {
public:
    static ProtocolFlagData* parse(const QString& templateStr, bool contentRequired = true);

private:
    struct Segment {
        QString type;
        QString data;

        bool isContentBegin = false;
        bool isLittleEndian = true;
    };

    static QList<Segment> parseToSegment(const QString& str);

private:
    static bool isLetter(const QChar& c);
    static bool isNumber(const QChar& c);
    static bool isComment(const QChar& c);
    static bool isReverse(const QChar& c);

    static ProtocolFlagData* parseHeader(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
    static ProtocolFlagData* parseAddress(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
    static ProtocolFlagData* parseType(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
    static ProtocolFlagData* parseSize(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
    static ProtocolFlagData* parseContent(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
    static ProtocolFlagData* parseVerify(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
    static ProtocolFlagData* parseEnd(const Segment& segment, ProtocolFlagData* prev, bool contentRequired);
};

PROTOCOL_CODEC_NAMESPACE_END