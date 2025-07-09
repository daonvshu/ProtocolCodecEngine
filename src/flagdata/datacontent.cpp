#include "flagdata/datacontent.h"

#include "flagdata/datasize.h"
#include "flagdata/dataverify.h"
#include "flagdata/dataend.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagDataContent::ProtocolFlagDataContent()
    : ProtocolFlagData(ProtocolFlag::Flag_Content)
    , byteSize(0)
{}

ProtocolFlagDataContent::ProtocolFlagDataContent(int byteSize)
    : ProtocolFlagData(ProtocolFlag::Flag_Content)
    , byteSize(byteSize)
{}

bool ProtocolFlagDataContent::verify(char *data, int offset, int maxSize, const QLoggingCategory& (*debugPtr)()) {
    if (!mSizeFlag.isNull()) {
        contentData = QByteArray(data + offset, mSizeFlag->dataSize);
    } else {
        if (byteSize <= 0) {
            return false;
        }
        contentData = QByteArray(data + offset, byteSize);
    }
    return true;
}

void ProtocolFlagDataContent::doFrameOffset(int &offset) {
    if (!mSizeFlag.isNull()) {
        offset += mSizeFlag->dataSize;
    } else {
        offset += byteSize;
    }
    if (!mVerifyFlag.isNull()) {
        offset += mVerifyFlag->byteSize;
    }
    if (!mEndFlag.isNull()) {
        offset += mEndFlag->target.size();
    }
}

void ProtocolFlagDataContent::setDependentFlag(const QSharedPointer<ProtocolFlagDataSize> &sizeFlag,
                                               const QSharedPointer<ProtocolFlagDataVerify> &verifyFlag,
                                               const QSharedPointer<ProtocolFlagDataEnd> &endFlag
) {
    mSizeFlag = sizeFlag;
    mVerifyFlag = verifyFlag;
    mEndFlag = endFlag;
}

QSharedPointer<ProtocolFlagData> ProtocolFlagDataContent::copy() const  {
    auto newFlag = QSharedPointer<ProtocolFlagDataContent>::create();
    newFlag->contentData = contentData;
    newFlag->byteSize = byteSize;
    return newFlag;
}

PROTOCOL_CODEC_NAMESPACE_END