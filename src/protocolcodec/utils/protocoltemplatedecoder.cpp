#include "protocoltemplatedecoder.h"

#include "flagdata/dataheader.h"
#include "flagdata/datacontent.h"
#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "flagdata/dataend.h"
#include "flagdata/dataaddress.h"
#include "flagdata/datatype.h"

#include "../protocolexception.h"

#include <qdebug.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagData* ProtocolTemplateDecoder::parse(const QString &templateStr, bool contentRequired) {
    ProtocolFlagData* next = nullptr;

    auto segments = parseToSegment(templateStr);
    for (const auto& segment : segments) {
        if (segment.type == "H") {
            next = parseHeader(segment, next, contentRequired);
        } else if (segment.type == "A") {
            next = parseAddress(segment, next, contentRequired);
        } else if (segment.type == "T") {
            next = parseType(segment, next, contentRequired);
        }  else if (segment.type == "S") {
            next = parseSize(segment, next, contentRequired);
        } else if (segment.type == "C") {
            next = parseContent(segment, next, contentRequired);
        } else if (segment.type == "V") {
            next = parseVerify(segment, next, contentRequired);
        } else if (segment.type == "E") {
            next = parseEnd(segment, next, contentRequired);
        } else {
            continue;
        }
        next->isLittleEndian = segment.isLittleEndian;
    }

    while (next != nullptr && next->prev != nullptr) {
        next = next->prev;
    }
    return next;
}

QList<ProtocolTemplateDecoder::Segment> ProtocolTemplateDecoder::parseToSegment(const QString &str) {
    QList<Segment> segments;

    Segment lastSegment;
    int len = str.length();
    for (int i = 0; i < len; i++) {
        auto c = str.at(i);

        if (isLetter(c)) {
            if (lastSegment.isContentBegin) {
                lastSegment.data.append(c);
            } else {
                lastSegment.type = c;
                lastSegment.data = QString();
                segments << lastSegment;
            }
            continue;
        }

        if (isNumber(c)) {
            if (lastSegment.isContentBegin) {
                lastSegment.data.append(c);
            } else {
                if (!lastSegment.type.isEmpty()) {
                    segments.last().data.append(c);
                } else {
                    throw ProtocolException(QLatin1String("number begin with null type:") + c);
                }
            }
            continue;
        }

        if (isComment(c)) {
            if (c == "(") {
                lastSegment.type = QString();
                lastSegment.data = QString();
                lastSegment.isContentBegin = true;
            } else {
                if (segments.isEmpty()) {
                    throw ProtocolException(QLatin1String("content with null type:") + lastSegment.data);
                } else {
                    segments.last().data = lastSegment.data;
                }
                lastSegment.isContentBegin = false;
            }
            continue;
        }

        if (isReverse(c)) {
            if (!lastSegment.type.isEmpty()) {
                segments.last().isLittleEndian = false;
            } else {
                throw ProtocolException(QLatin1String("big-endian begin with null type:") + c);
            }
            continue;
        }

        throw ProtocolException(QLatin1String("unknown snippet char:") + c);
    }

    return segments;
}

bool ProtocolTemplateDecoder::isLetter(const QChar &c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool ProtocolTemplateDecoder::isNumber(const QChar& c) {
    return (c >= '0' && c <= '9') || c == '.' || c == '-';
}

bool ProtocolTemplateDecoder::isComment(const QChar &c) {
    return c == '(' || c == ')';
}

bool ProtocolTemplateDecoder::isReverse(const QChar& c) {
    return c == '!';
}

ProtocolFlagData* ProtocolTemplateDecoder::parseHeader(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    if (contentRequired && segment.data.isEmpty()) {
        throw ProtocolException("header required content!");
    }
    auto data = new ProtocolFlagDataHeader(QByteArray::fromHex(segment.data.toLatin1()));
    data->bindPrev(prev);
    return data;
}

ProtocolFlagData* ProtocolTemplateDecoder::parseAddress(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    if ((contentRequired && segment.data.isEmpty()) || segment.data.toInt() < 0) {
        throw ProtocolException("address required content!");
    }
    auto data = new ProtocolFlagDataAddress(segment.data.toInt());
    data->bindPrev(prev);
    return data;
}

ProtocolFlagData* ProtocolTemplateDecoder::parseType(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    if (contentRequired && segment.data.isEmpty()) {
        throw ProtocolException("type required content!");
    }
    auto data = new ProtocolFlagDataType(segment.data.toInt());
    data->bindPrev(prev);
    return data;
}

ProtocolFlagData* ProtocolTemplateDecoder::parseSize(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    if (contentRequired && segment.data.isEmpty()) {
        throw ProtocolException("size required content!");
    }
    auto data = new ProtocolFlagDataSize(segment.data.toInt());
    data->bindPrev(prev);
    return data;
}

ProtocolFlagData* ProtocolTemplateDecoder::parseContent(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    ProtocolFlagData* data;
    if (segment.data.isEmpty()) {
        data = new ProtocolFlagDataContent;
    } else {
        data = new ProtocolFlagDataContent(segment.data.toInt());
    }
    data->bindPrev(prev);
    return data;
}

ProtocolFlagData* ProtocolTemplateDecoder::parseVerify(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    ProtocolFlagData* data;
    if (contentRequired) {
        if (segment.data.toUpper() == "CRC16") {
            data = new ProtocolFlagDataVerify(ProtocolFlagDataVerify::Crc16);
        } else if (segment.data.toUpper() == "SUM8") {
            data = new ProtocolFlagDataVerify(ProtocolFlagDataVerify::Sum8);
        } else if (segment.data.toUpper() == "SUM16") {
            data = new ProtocolFlagDataVerify(ProtocolFlagDataVerify::Sum16);
        } else {
            throw ProtocolException("unknown verify type:" + segment.data);
        }
    } else {
        data = new ProtocolFlagDataVerify;
    }
    data->bindPrev(prev);
    return data;
}

ProtocolFlagData* ProtocolTemplateDecoder::parseEnd(const Segment& segment, ProtocolFlagData* prev, bool contentRequired) {
    if (contentRequired && segment.data.isEmpty()) {
        throw ProtocolException("tail required content!");
    }
    auto data = new ProtocolFlagDataEnd(QByteArray::fromHex(segment.data.toLatin1()));
    data->bindPrev(prev);
    return data;
}

PROTOCOL_CODEC_NAMESPACE_END
