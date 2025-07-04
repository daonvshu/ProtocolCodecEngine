package com.daonvshu.protocol.codec.utils

import com.daonvshu.protocol.codec.ProtocolFlag

interface ProtocolFlagData {

    val flagType: ProtocolFlag

    fun verify(data: ByteArray, offset: Int, maxSize: Int)

    fun doFrameOffset(offset: Int): Int
}