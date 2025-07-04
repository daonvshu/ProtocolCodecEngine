package com.daonvshu.protocol.codec

import com.daonvshu.protocol.codec.annotations.Subscribe
import com.daonvshu.protocol.codec.annotations.Type
import com.daonvshu.protocol.codec.codec.ProtocolDecoder
import com.daonvshu.protocol.codec.codec.ProtocolEncoder
import com.daonvshu.protocol.codec.utils.ProtocolTemplateDecoder
import java.lang.reflect.Method

class ProtocolCodecEngine {

    val decoder = ProtocolDecoder()
    private val encoder = ProtocolEncoder()

    private lateinit var context: Class<*>

    fun frameDeclare(templateStr: String) {
        val flags = ProtocolTemplateDecoder.parse(templateStr)
        decoder.setFlags(flags)
        encoder.setFlags(flags)
    }

    fun setVerifyFlags(flagStr: String) {
        val flags = ProtocolTemplateDecoder.parse(flagStr)
        decoder.setVerifyFlags(flags)
        encoder.setVerifyFlags(flags)
    }

    fun setTypeEncodeByteSize(size: Int) {

    }

    fun setSizeMaxValue(value: Int) {

    }

    fun setBufferMaxSize(size: Int) {

    }

    fun setDecodeTimeout(ms: Int) {

    }

    fun registerCallback(context: Class<*>) {
        this.context = context
    }

    inline fun<reified T> registerType() {
        val kClass = T::class
        val typeAnnotation = kClass.annotations.firstOrNull { it is Type } as? Type
        if (typeAnnotation == null) {
            throw IllegalArgumentException("Class ${kClass.simpleName} must be annotated with @Type!")
        }
        val codec: ProtocolTypeCodec<out Any?> = when(typeAnnotation.codec) {
            CodecType.JSON -> JsonCodec(kClass.java)
            CodecType.Bytes -> BytesCodec(kClass.java)
        }
        decoder.typeDecoders[typeAnnotation.value] = {
            val data = codec.decode(it)
            val method = findMethodByType(T::class.java)
            method.invoke(data)
        }
    }

    fun appendBuffer(buffer: ByteArray) {

    }

    fun<T> findMethodByType(clazz: Class<T>): Method {
        if (!::context.isInitialized) {
            throw IllegalStateException("Context is not register!")
        }
        context.methods.forEach { method ->
            val annotation = method.getAnnotation(Subscribe::class.java)
            if (annotation != null) {
                method.parameters.forEach { parameter ->
                    if (parameter.type == clazz) {
                        return method
                    }
                }
            }
        }
        throw IllegalArgumentException("Can not find subscribe method for $clazz with param type: ${clazz.simpleName}")
    }
}