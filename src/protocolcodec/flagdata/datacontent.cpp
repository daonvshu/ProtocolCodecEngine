#include "datacontent.h"
#include "datasize.h"
#include "datatype.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagDataContent::ProtocolFlagDataContent()
    : ProtocolFlagData(ProtocolFlag::Flag_Content, 0)
    , isFixByteSize(false)
{
}

ProtocolFlagDataContent::ProtocolFlagDataContent(int byteSize)
    : ProtocolFlagData(ProtocolFlag::Flag_Content, byteSize)
    , isFixByteSize(true)
{}

void ProtocolFlagDataContent::setContentSizeFromType(const QMap<int, int>& sizeFromTypeMap) {
    contentSizeFromType = sizeFromTypeMap;
}

bool ProtocolFlagDataContent::verify(char *data, int baseOffset, int maxSize, ProtocolMetaData* metaData, const QLoggingCategory& (*debugPtr)()) {
    const qint64 curDataOffset = qint64(baseOffset) + getCurrentByteOffset();
    const int contentSize = getByteSize();
    if (contentSize < 0 || curDataOffset < 0 || curDataOffset > maxSize
        || contentSize > maxSize - curDataOffset) {
        return false;
    }
    contentData = QByteArray(data + curDataOffset, contentSize);
    if (metaData) {
        metaData->content = contentData;
    }
    return true;
}

int ProtocolFlagDataContent::getByteSize() const {
    if (isFixByteSize) {
        return byteSize;
    }
    if (isSizeTarget) {
        auto prevPtr = this->prev;
        while (prevPtr != nullptr) {
            if (prevPtr->flag == ProtocolFlag::Flag_Size) {
                return dynamic_cast<ProtocolFlagDataSize*>(prevPtr)->getContentSize();
            }
            prevPtr = prevPtr->prev;
        }
        qFatal("Cannot get content byte size! size flag must be defined beforehand.");
    }
    //get byte size by type
    auto prevPtr = this->prev;
    while (prevPtr != nullptr) {
        if (prevPtr->flag == ProtocolFlag::Flag_Type) {
            int curType = dynamic_cast<ProtocolFlagDataType*>(prevPtr)->value;
            return contentSizeFromType.value(curType, -1);
        }
        prevPtr = prevPtr->prev;
    }
    return -1;
}

QByteArray ProtocolFlagDataContent::getBytesContent() const {
    return contentData;
}

PROTOCOL_CODEC_NAMESPACE_END
