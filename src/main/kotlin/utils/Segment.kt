package com.daonvshu.protocol.codec.utils

data class Segment(
    val type: Char? = null,
    var data: StringBuilder = StringBuilder(),
    var isContentBegin: Boolean = false
)
