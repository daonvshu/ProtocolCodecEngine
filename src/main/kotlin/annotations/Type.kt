package com.daonvshu.protocol.codec.annotations

import com.daonvshu.protocol.codec.CodecType

@Target(AnnotationTarget.CLASS)
@Retention(AnnotationRetention.RUNTIME)
annotation class Type(val value: Int, val codec: CodecType)
