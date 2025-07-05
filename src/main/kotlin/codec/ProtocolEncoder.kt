package com.daonvshu.protocol.codec.codec

import com.daonvshu.protocol.codec.ProtocolFlag.*
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataEnd
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataHeader
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataSize
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataVerify
import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

class ProtocolEncoder : ProtocolCodec() {

    val typeEncoders = mutableMapOf<Int, (Any) -> ByteArray>()

    fun encodeFrame(data: ByteArray, dataType: Int): ByteArray {
        val buffer = ByteArrayOutputStream()
        var contentOffset = 0

        for (flag in flags) {
            when (flag.flagType) {
                FlagHeader -> {
                    val headerFlag = flag as ProtocolFlagDataHeader
                    buffer.write(headerFlag.target)
                }

                FlagSize -> {
                    val sizeFlag = flag as ProtocolFlagDataSize
                    val contentSize = data.size + typeByteSize

                    // 低位优先字节序写入contentSize
                    val sizeBuff = ByteArray(sizeFlag.byteSize)
                    for (i in 0 until sizeFlag.byteSize) {
                        sizeBuff[i] = ((contentSize shr (i * 8)) and 0xFF).toByte()
                    }
                    buffer.write(sizeBuff)
                    sizeFlag.dataSize = contentSize
                }

                FlagContent -> {
                    contentOffset = buffer.size()
                    // 写入dataType，低位优先（小端）
                    val typeBytes = ByteBuffer.allocate(typeByteSize)
                        .order(ByteOrder.LITTLE_ENDIAN)
                        .putInt(dataType)
                        .array()
                    buffer.write(typeBytes)
                    if (data.isNotEmpty()) {
                        buffer.write(data)
                    }
                }

                FlagVerify -> {
                    val verifyFlag = flag as ProtocolFlagDataVerify
                    val currentBuffer = buffer.toByteArray()
                    val verifyCode = verifyFlag.getVerifyCode(currentBuffer, contentOffset)
                    buffer.write(verifyCode)
                }

                FlagEnd -> {
                    val endFlag = flag as ProtocolFlagDataEnd
                    buffer.write(endFlag.target)
                }

                FlagMax -> {}
            }
        }

        return buffer.toByteArray()
    }
}