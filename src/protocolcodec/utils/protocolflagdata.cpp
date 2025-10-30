#include "protocolflagdata.h"

#include "flagdata/datacontent.h"

PROTOCOL_CODEC_NAMESPACE_BEGIN

ProtocolFlagData::ProtocolFlagData(ProtocolFlag flag, int byteSize)
    : flag(flag)
    , byteSize(byteSize)
{}

QString ProtocolFlagData::dataToString() {
    return {};
}

void ProtocolFlagData::bindPrev(ProtocolFlagData* prevPtr) {
    if (prevPtr != nullptr) {
        prevPtr->next = this;
    }
    this->prev = prevPtr;
}

int ProtocolFlagData::getCurrentByteOffset() const {
    int byteSizeOffset = 0;
    auto prevPtr = prev;
    while (prevPtr != nullptr) {
        if (prevPtr->flag == ProtocolFlag::Flag_Content) {
            byteSizeOffset += dynamic_cast<ProtocolFlagDataContent*>(prevPtr)->getByteSize();
        } else {
            byteSizeOffset += prevPtr->byteSize;
        }
        prevPtr = prevPtr->prev;
    }
    return byteSizeOffset;
}

PROTOCOL_CODEC_NAMESPACE_END



