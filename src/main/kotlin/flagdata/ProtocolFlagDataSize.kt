package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataSize(byteSize: Int) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagSize

    override fun verify(data: ByteArray, offset: Int, maxSize: Int) {
        TODO("Not yet implemented")
    }

    override fun doFrameOffset(offset: Int): Int {
        TODO("Not yet implemented")
    }

    fun setSizeMaxValue(value: Int) {
        TODO("Not yet implemented")
    }
}