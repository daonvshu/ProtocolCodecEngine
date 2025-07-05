package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolException
import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataEnd(val target: ByteArray) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagEnd

    private var sizeFlag: ProtocolFlagDataSize? = null
    private var contentFlag: ProtocolFlagDataContent? = null
    private var verifyFlag: ProtocolFlagDataVerify? = null

    override fun verify(data: ByteArray, offset: Int, maxSize: Int): Boolean {
        var endOffset = offset
        endOffset += if (sizeFlag != null) {
            sizeFlag!!.dataSize
        } else {
            contentFlag!!.byteSize
        }
        if (verifyFlag != null) {
            endOffset += verifyFlag!!.byteSize
        }
        if (endOffset <= offset) {
            return false
        }
        if (endOffset + target.size > maxSize) {
            return false
        }
        for (i in 0 until target.size) {
            if (data[endOffset + i] != target[i]) {
                return false
            }
        }
        return true
    }

    override fun doFrameOffset(offset: Int): Int {
        return offset
    }

    fun setDependentFlag(sizeFlag: ProtocolFlagDataSize?, contentFlag: ProtocolFlagDataContent?, verifyFlag: ProtocolFlagDataVerify?) {
        this.sizeFlag = sizeFlag
        this.contentFlag = contentFlag
        this.verifyFlag = verifyFlag
        if (sizeFlag == null && contentFlag == null) {
            throw ProtocolException("frame end decode require 'size' and 'content' byte size!")
        }
    }
}