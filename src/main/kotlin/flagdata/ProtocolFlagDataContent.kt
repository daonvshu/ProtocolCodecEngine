package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataContent(val byteSize: Int = 0) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagContent

    private var sizeFlag: ProtocolFlagDataSize? = null
    private var verifyFlag: ProtocolFlagDataVerify? = null
    private var endFlag: ProtocolFlagDataEnd? = null

    var contentData = ByteArray(0)

    override fun verify(data: ByteArray, offset: Int, maxSize: Int): Boolean {
        if (sizeFlag != null) {
            contentData = data.copyOfRange(offset, offset + sizeFlag!!.dataSize)
        } else {
            if (byteSize <= 0) {
                return false
            }
            contentData = data.copyOfRange(offset, offset + byteSize)
        }
        return true
    }

    override fun doFrameOffset(offset: Int): Int {
        var nextOffset = offset
        nextOffset += if (sizeFlag != null) {
            sizeFlag!!.dataSize
        } else {
            byteSize
        }
        nextOffset += if (verifyFlag != null) {
            verifyFlag!!.byteSize
        } else {
            endFlag!!.target.size
        }
        return nextOffset
    }

    fun setDependentFlag(sizeFlag: ProtocolFlagDataSize?, verifyFlag: ProtocolFlagDataVerify?, endFlag: ProtocolFlagDataEnd?) {
        this.sizeFlag = sizeFlag
        this.verifyFlag = verifyFlag
        this.endFlag = endFlag
    }
}