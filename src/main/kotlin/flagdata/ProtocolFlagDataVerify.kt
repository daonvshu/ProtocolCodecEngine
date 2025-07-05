package com.daonvshu.protocol.codec.flagdata

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.utils.ProtocolFlagData
import java.nio.ByteBuffer
import java.nio.ByteOrder

enum class VerifyType {
    Crc16,
    Sum8,
    Sum16,
}

class ProtocolFlagDataVerify(val verifyType: VerifyType) : ProtocolFlagData {

    override val flagType = ProtocolFlag.FlagVerify

    private val verifyFlags = mutableListOf<ProtocolFlagData?>()

    val byteSize: Int = when (verifyType) {
        VerifyType.Crc16 -> 2
        VerifyType.Sum8 -> 1
        VerifyType.Sum16 -> 2
    }

    override fun verify(data: ByteArray, offset: Int, maxSize: Int): Boolean {
        var verifyBegin = offset
        var verifySize = 0
        var verifyCodeIndex: Int? = null

        for (flag in verifyFlags) {
            when (flag?.flagType) {
                ProtocolFlag.FlagHeader -> {
                    val headerFlag = flag as ProtocolFlagDataHeader
                    verifyBegin -= headerFlag.target.size
                    verifySize += headerFlag.target.size
                }

                ProtocolFlag.FlagSize -> {
                    val sizeFlag = flag as ProtocolFlagDataSize
                    verifyBegin -= sizeFlag.byteSize
                    verifySize += sizeFlag.byteSize + sizeFlag.dataSize
                    verifyCodeIndex = offset + sizeFlag.dataSize
                }

                else -> {}
            }
        }

        if (verifyBegin < 0 || verifySize <= 0) {
            return false
        }
        if (verifyCodeIndex == null) {
            return false
        }
        if (verifyCodeIndex + 2 > data.size) return false // 防止越界

        return when (verifyType) {
            VerifyType.Crc16 -> {
                val curVerify = data.copyOfRange(verifyBegin, verifyBegin + verifySize).crc16()
                val verifyCode = ((data[verifyCodeIndex].toInt() and 0xff) or
                        ((data[verifyCodeIndex + 1].toInt() and 0xff) shl 8)).toUShort()
                curVerify == verifyCode
            }

            VerifyType.Sum8 -> {
                val curVerify = data.copyOfRange(verifyBegin, verifyBegin + verifySize).sumCheck()
                val verifyCode = data[verifyCodeIndex].toUByte()
                curVerify == verifyCode
            }

            VerifyType.Sum16 -> {
                val curVerify = data.copyOfRange(verifyBegin, verifyBegin + verifySize).sumCheck2()
                val verifyCode = ((data[verifyCodeIndex].toInt() and 0xff) or
                        ((data[verifyCodeIndex + 1].toInt() and 0xff) shl 8)).toUShort()
                curVerify == verifyCode
            }
        }
    }

    fun getVerifyCode(buff: ByteArray, contentOffset: Int): ByteArray {
        var verifyBegin = contentOffset
        var verifySize = 0

        for (flag in verifyFlags) {
            when (flag?.flagType) {
                ProtocolFlag.FlagHeader -> {
                    val headerFlag = flag as ProtocolFlagDataHeader
                    verifyBegin -= headerFlag.target.size
                    verifySize += headerFlag.target.size
                }

                ProtocolFlag.FlagSize -> {
                    val sizeFlag = flag as ProtocolFlagDataSize
                    verifyBegin -= sizeFlag.byteSize
                    verifySize += sizeFlag.byteSize + sizeFlag.dataSize
                }

                else -> {
                    // 其他标志不处理
                }
            }
        }

        val start = verifyBegin.coerceAtLeast(0)
        val endExclusive = (verifyBegin + verifySize).coerceAtMost(buff.size)
        val subBuff = if (start < endExclusive) buff.copyOfRange(start, endExclusive) else byteArrayOf()

        return when (verifyType) {
            VerifyType.Crc16 -> {
                val curVerify = subBuff.crc16()
                // 将 UInt16 转成两个字节的小端数组
                ByteBuffer.allocate(2)
                    .order(ByteOrder.LITTLE_ENDIAN)
                    .putShort(curVerify.toShort())
                    .array()
            }

            VerifyType.Sum8 -> {
                val curVerify = subBuff.sumCheck()
                byteArrayOf(curVerify.toByte())
            }

            VerifyType.Sum16 -> {
                val curVerify = subBuff.sumCheck2()
                ByteBuffer.allocate(2)
                    .order(ByteOrder.LITTLE_ENDIAN)
                    .putShort(curVerify.toShort())
                    .array()
            }
        }
    }

    override fun doFrameOffset(offset: Int): Int {
        return offset
    }

    fun setVerifyFlags(flags: List<ProtocolFlagData?>) {
        verifyFlags.clear()
        verifyFlags.addAll(flags)
    }
}

/**
 * 和校验，返回UByte类型（等价于uchar）
 */
fun ByteArray.sumCheck(length: Int = this.size): UByte {
    var sum: UByte = 0u
    for (i in 0 until length.coerceAtMost(this.size)) {
        sum = (sum + this[i].toUByte()).toUByte()
    }
    return sum
}

/**
 * 和校验，返回UShort类型
 */
fun ByteArray.sumCheck2(length: Int = this.size): UShort {
    var sum: UShort = 0u
    for (i in 0 until length.coerceAtMost(this.size)) {
        sum = (sum + (this[i].toUByte()).toUShort()).toUShort()
    }
    return sum
}

/**
 * CRC16校验（Modbus多为0xA001，多为低字节在前）
 */
fun ByteArray.crc16(length: Int = this.size): UShort {
    var crc: UShort = 0xFFFFu
    for (i in 0 until length.coerceAtMost(this.size)) {
        crc = (crc.toInt() xor (this[i].toInt() and 0xFF)).toUShort()
        repeat(8) {
            crc = if ((crc.toInt() and 0x01) != 0)
                ((crc.toInt() shr 1) xor 0xA001).toUShort()
            else
                (crc.toInt() shr 1).toUShort()
        }
    }
    return crc
}