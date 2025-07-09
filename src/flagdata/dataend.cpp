#include "flagdata/dataend.h"

#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"

#include "protocolexception.h"
#include "flagdata/datacontent.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN
    ProtocolFlagDataEnd::ProtocolFlagDataEnd(QByteArray data)
    : ProtocolFlagData(ProtocolFlag::Flag_End)
    , target(std::move(data))
{}

QString ProtocolFlagDataEnd::dataToString() {
    return target.toHex(' ');
}

bool ProtocolFlagDataEnd::verify(char *data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) {
    auto endOffset = offset;
    if (!mSizeFlag.isNull()) {
        endOffset += mSizeFlag->dataSize;
    } else {
        endOffset += mContentFlag->byteSize;
    }
    if (!mVerifyFlag.isNull()) {
        endOffset += mVerifyFlag->byteSize;
    }
    if (endOffset <= offset) {
        return false;
    }
    if (endOffset + target.size() > maxSize) {
        return false;
    }
    for (int i = 0; i < target.size(); i++) {
        if (data[i + endOffset] != target[i]) {
            return false;
        }
    }
    return true;
}

void ProtocolFlagDataEnd::doFrameOffset(int &offset) {
}

void ProtocolFlagDataEnd::setDependentFlag(const QSharedPointer<ProtocolFlagDataSize> &sizeFlag,
                                           const QSharedPointer<ProtocolFlagDataContent>& contentFlag,
                                           const QSharedPointer<ProtocolFlagDataVerify> &verifyFlag)
{
    mSizeFlag = sizeFlag;
    mContentFlag = contentFlag;
    mVerifyFlag = verifyFlag;
    if (mSizeFlag.isNull() && mContentFlag.isNull()) {
        throw ProtocolException("frame end decode require 'size' and 'content' byte size!");
    }
}

QSharedPointer<ProtocolFlagData> ProtocolFlagDataEnd::copy() const {
    return QSharedPointer<ProtocolFlagDataEnd>::create(target);
}

PROTOCOL_CODEC_NAMESPACE_END