package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

enum class VerifyType {
    Crc16,
    Sum8,
    Sum16,
}

class ProtocolFlagDataVerify(verifyType: VerifyType) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagVerify

    private val verifyFlags = mutableListOf<ProtocolFlagData?>()

    override fun verify(data: ByteArray, offset: Int, maxSize: Int) {
        TODO("Not yet implemented")
    }

    override fun doFrameOffset(offset: Int): Int {
        TODO("Not yet implemented")
    }

    fun setVerifyFlags(flags: List<ProtocolFlagData?>) {
        verifyFlags.clear()
        verifyFlags.addAll(flags)
    }
}