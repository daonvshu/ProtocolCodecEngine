package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolFlagDataContent(byteSize: Int = 0) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagContent

    private var sizeFlag: ProtocolFlagDataSize? = null
    private var verifyFlag: ProtocolFlagDataVerify? = null
    private var endFlag: ProtocolFlagDataEnd? = null

    override fun verify(data: ByteArray, offset: Int, maxSize: Int) {
        TODO("Not yet implemented")
    }

    override fun doFrameOffset(offset: Int): Int {
        TODO("Not yet implemented")
    }

    fun setDependentFlag(sizeFlag: ProtocolFlagDataSize?, verifyFlag: ProtocolFlagDataVerify?, endFlag: ProtocolFlagDataEnd?) {
        this.sizeFlag = sizeFlag
        this.verifyFlag = verifyFlag
        this.endFlag = endFlag
    }
}