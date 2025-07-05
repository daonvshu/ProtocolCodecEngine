package com.daonvshu.protocol.codec.utils

import com.daonvshu.protocol.codec.ProtocolException
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataContent
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataEnd
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataHeader
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataSize
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataVerify
import com.daonvshu.protocol.codec.flagdata.VerifyType

object ProtocolTemplateDecoder {

    @Throws(ProtocolException::class)
    fun parse(templateStr: String, contentRequired: Boolean = true): List<ProtocolFlagData> {
        val data = mutableListOf<ProtocolFlagData>()

        val segments = parseToSegment(templateStr)
        for (segment in segments) {
            when (segment.type) {
                'H' -> {
                    if (contentRequired && segment.data.isEmpty()) {
                        throw ProtocolException("header required content!")
                    }
                    data.add(ProtocolFlagDataHeader(hexStringToByteArray(segment.data.toString())))
                }
                'S' -> {
                    if (contentRequired && segment.data.isEmpty()) {
                        throw ProtocolException("size required content!")
                    }
                    data.add(ProtocolFlagDataSize(segment.data.toString().toIntOrNull() ?: 0))
                }
                'C' -> {
                    if (segment.data.isEmpty()) {
                        data.add(ProtocolFlagDataContent())
                    } else {
                        data.add(ProtocolFlagDataContent(segment.data.toString().toIntOrNull() ?: 0))
                    }
                }
                'V' -> {
                    if (contentRequired) {
                        when (segment.data.toString().uppercase()) {
                            "CRC16" -> data.add(ProtocolFlagDataVerify(VerifyType.Crc16))
                            "SUM8" -> data.add(ProtocolFlagDataVerify(VerifyType.Sum8))
                            "SUM16" -> data.add(ProtocolFlagDataVerify(VerifyType.Sum16))
                            else -> throw ProtocolException("unknown verify type: ${segment.data}")
                        }
                    }
                }
                'E' -> {
                    if (contentRequired && segment.data.isEmpty()) {
                        throw ProtocolException("tail required content!")
                    }
                    data.add(ProtocolFlagDataEnd(hexStringToByteArray(segment.data.toString())))
                }
                else -> {
                }
            }
        }

        return data
    }

    @Throws(ProtocolException::class)
    private fun parseToSegment(str: String): List<Segment> {
        val segments = mutableListOf<Segment>()
        var lastSegment = Segment()
        val len = str.length

        for (i in 0 until len) {
            val c = str[i]

            when {
                isLetter(c) -> {
                    if (lastSegment.isContentBegin) {
                        lastSegment.data.append(c)
                    } else {
                        lastSegment = Segment(type = c, data = StringBuilder())
                        segments.add(lastSegment)
                    }
                }
                isNumber(c) -> {
                    if (lastSegment.isContentBegin) {
                        lastSegment.data.append(c)
                    } else {
                        if (lastSegment.type != null) {
                            segments.last().data.append(c)
                        } else {
                            throw ProtocolException("number begin with null type: $c")
                        }
                    }
                }
                isComment(c) -> {
                    if (c == '(') {
                        lastSegment = Segment(type = null, data = StringBuilder(), isContentBegin = true)
                    } else { // c == ')'
                        if (segments.isEmpty()) {
                            throw ProtocolException("content with null type: ${lastSegment.data}")
                        } else {
                            segments.last().data = lastSegment.data
                        }
                        lastSegment.isContentBegin = false
                    }
                }
                else -> throw ProtocolException("unknown snippet char: $c")
            }
        }

        return segments
    }

    private fun isLetter(c: Char): Boolean {
        return c in 'A'..'Z' || c in 'a'..'z'
    }

    private fun isNumber(c: Char): Boolean {
        return c in '0'..'9' || c == '.' || c == '-'
    }

    private fun isComment(c: Char): Boolean {
        return c == '(' || c == ')'
    }

    private fun hexStringToByteArray(s: String): ByteArray {
        if (s.isEmpty()) {
            return ByteArray(0)
        }
        val cleanString = s.replace("\\s".toRegex(), "")
        if (cleanString.length % 2 != 0) {
            throw ProtocolException("Invalid hex string length")
        }
        return ByteArray(cleanString.length / 2) {
            cleanString.substring(it*2, it*2 + 2).toInt(16).toByte()
        }
    }
}