package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataHeader(val target: ByteArray) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagHeader

    override fun verify(data: ByteArray, offset: Int, maxSize: Int): Boolean {
        if (offset + target.size > maxSize) {
            return false
        }
        for (i in target.indices) {
            if (target[i] != data[offset + i]) {
                return false
            }
        }
        return true
    }

    override fun doFrameOffset(offset: Int): Int {
        return offset + target.size
    }
}