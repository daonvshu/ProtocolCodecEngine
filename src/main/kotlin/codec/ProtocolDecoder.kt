package com.daonvshu.protocol.codec.codec

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataSize
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolDecoder : ProtocolCodec() {

    val typeDecoders = mutableMapOf<Int, (ByteArray) -> Unit>()

    val decodeFlags = mutableListOf<ProtocolFlag>()

    var bufferCache = ByteArray(0)

    var bufferMaxSize = 10 * 1024 * 1024 //10M

    var decodeTimeout = -1

    val validHeaderPos = mutableListOf<Int>()

    var lastDecodeTimestamp = 0L

    override fun setFlags(flagList: List<ProtocolFlagData>) {
        super.setFlags(flagList)

        flagList.forEach {
            decodeFlags.add(it.flagType)
        }

        val flagWeight = mapOf(
            ProtocolFlag.FlagHeader to 100,
            ProtocolFlag.FlagSize to 99,
            ProtocolFlag.FlagEnd to 98,
            ProtocolFlag.FlagVerify to 97,
            ProtocolFlag.FlagContent to 96
        )
        decodeFlags.sortBy { flagWeight[it] }
    }

    fun setBufferMaxSize(size: Int) {
        bufferMaxSize = size
    }

    fun setSizeMaxValue(value: Int) {
        get<ProtocolFlagDataSize>(ProtocolFlag.FlagSize)?.setSizeMaxValue(value)
    }

    fun setDecodeTimeout(ms: Int) {
        decodeTimeout = ms
    }

    fun addBuffer(buffer: ByteArray) {
        if (lastDecodeTimestamp > 0) {

        }
    }
}