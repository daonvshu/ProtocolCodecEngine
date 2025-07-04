package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataEnd(target: ByteArray) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagEnd

    private var sizeFlag: ProtocolFlagDataSize? = null
    private var contentFlag: ProtocolFlagDataContent? = null
    private var verifyFlag: ProtocolFlagDataVerify? = null

    override fun verify(data: ByteArray, offset: Int, maxSize: Int) {
        TODO("Not yet implemented")
    }

    override fun doFrameOffset(offset: Int): Int {
        TODO("Not yet implemented")
    }

    fun setDependentFlag(sizeFlag: ProtocolFlagDataSize?, contentFlag: ProtocolFlagDataContent?, verifyFlag: ProtocolFlagDataVerify?) {
        this.sizeFlag = sizeFlag
        this.contentFlag = contentFlag
        this.verifyFlag = verifyFlag
    }
}