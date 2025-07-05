package com.daonvshu.protocol.codec.codec

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataContent
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataHeader
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataSize
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

class ProtocolDecoder : ProtocolCodec() {

    val typeDecoders = mutableMapOf<Int, (ByteArray) -> Unit>()

    val decodeFlags = mutableListOf<ProtocolFlag>()

    var bufferCache = ByteArray(0)

    var bufferMaxSize = 10 * 1024 * 1024 //10M

    var decodeTimeout = -1

    var validHeaderPos = mutableListOf<Int>()

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
        decodeFlags.reverse()
    }

    fun setSizeMaxValue(value: Int) {
        get<ProtocolFlagDataSize>(ProtocolFlag.FlagSize)?.sizeValueMax = value
    }

    fun addBuffer(buffer: ByteArray) {
        if (decodeTimeout > 0) {
            val cur = System.currentTimeMillis()
            if (cur - lastDecodeTimestamp > decodeTimeout) {
                bufferCache = ByteArray(0)
                validHeaderPos.clear()
            }
            lastDecodeTimestamp = cur
        }
        bufferCache += buffer

        var dataOffset = 0
        val dataSize = bufferCache.size
        var decodedOffset = 0

        var lastCheckHeaderIndex = 0
        val maxCheckHeaderSize = validHeaderPos.size
        while (dataOffset < dataSize) {
            if (lastCheckHeaderIndex < maxCheckHeaderSize) {
                dataOffset = validHeaderPos[lastCheckHeaderIndex++]
            }
            var frameOffset = dataOffset
            var frameDecodeSuccess = true
            for (flagType in decodeFlags) {
                val flag = flagMap[flagType.ordinal]
                val segmentValid = flag?.verify(bufferCache, frameOffset, dataSize) ?: false
                if (segmentValid) {
                    frameOffset = flag.doFrameOffset(frameOffset)
                    if (flagType == ProtocolFlag.FlagHeader) {
                        validHeaderPos.add(dataOffset)
                    }
                } else {
                    if (flagType == ProtocolFlag.FlagSize) {
                        validHeaderPos.removeLast()
                    }
                    frameDecodeSuccess = false
                    break
                }
            }
            if (!frameDecodeSuccess) {
                dataOffset++
            } else {
                decodedOffset = frameOffset
                dataOffset = frameOffset
                while (lastCheckHeaderIndex < maxCheckHeaderSize) {
                    if (validHeaderPos[lastCheckHeaderIndex] < decodedOffset) {
                        lastCheckHeaderIndex++
                    } else {
                        break
                    }
                }

                val content = get<ProtocolFlagDataContent>(ProtocolFlag.FlagContent)?.contentData
                if (content != null) {
                    var type = 0
                    for (i in 0 until typeByteSize) {
                        type = type or ((content[i].toInt() and 0xFF) shl (8 * i))
                    }
                    val decoder = typeDecoders[type]
                    if (decoder != null) {
                        decoder(content.copyOfRange(typeByteSize, content.size))
                    } else {
                        println("cannot find decoder to decode frame type: $type")
                    }
                }
            }
        }

        validHeaderPos = validHeaderPos.drop(maxCheckHeaderSize).toMutableList()
        if (validHeaderPos.isEmpty()) {
            val header = get<ProtocolFlagDataHeader>(ProtocolFlag.FlagHeader)
            bufferCache = if (header == null) {
                ByteArray(0)
            } else {
                bufferCache.takeLast(header.target.size).toByteArray()
            }
            return
        }
        decodedOffset = maxOf(decodedOffset, validHeaderPos.first())
        if (decodedOffset != 0) {
            bufferCache = bufferCache.drop(decodedOffset).toByteArray()
            for (i in validHeaderPos.indices) {
                validHeaderPos[i] -= decodedOffset
            }
        }
        if (bufferMaxSize > 0) {
            if (bufferCache.size > bufferMaxSize) {
                val offset = bufferCache.size - bufferMaxSize
                bufferCache = bufferCache.drop(offset).toByteArray()
                for (i in validHeaderPos.indices) {
                    validHeaderPos[i] -= offset
                }
            }
        }
        validHeaderPos.removeIf { it < 0 }
        if (validHeaderPos.isEmpty()) {
            val header = get<ProtocolFlagDataHeader>(ProtocolFlag.FlagHeader)
            bufferCache = if (header == null) {
                ByteArray(0)
            } else {
                bufferCache.takeLast(header.target.size).toByteArray()
            }
        }
    }
}