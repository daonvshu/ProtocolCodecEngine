package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataHeader(target: ByteArray) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagHeader

    override fun verify(data: ByteArray, offset: Int, maxSize: Int) {
        TODO("Not yet implemented")
    }

    override fun doFrameOffset(offset: Int): Int {
        TODO("Not yet implemented")
    }
}