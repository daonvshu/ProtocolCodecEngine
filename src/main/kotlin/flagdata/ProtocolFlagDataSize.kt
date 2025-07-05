package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataSize(val byteSize: Int) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagSize

    var dataSize = 0

    var sizeValueMax = -1

    override fun verify(data: ByteArray, offset: Int, maxSize: Int): Boolean {
        if (offset + byteSize > maxSize) {
            return false
        }
        dataSize = 0
        for (i in 0 until byteSize) {
            dataSize = dataSize or ((data[offset + i].toInt() and 0xFF) shl (i * 8))
        }
        if (sizeValueMax > 0) {
            if (dataSize > sizeValueMax) {
                return false
            }
        }
        return dataSize > 0
    }

    override fun doFrameOffset(offset: Int): Int {
        return offset + byteSize
    }
}