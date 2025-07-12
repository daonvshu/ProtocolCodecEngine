package com.daonvshu.protocol.codec

import com.daonvshu.protocol.codec.annotations.Subscribe
import com.daonvshu.protocol.codec.annotations.Type
import com.daonvshu.protocol.codec.codec.ProtocolDecoder
import com.daonvshu.protocol.codec.codec.ProtocolEncoder
import com.daonvshu.protocol.codec.utils.ProtocolTemplateDecoder
import kotlin.reflect.KClass
import kotlin.reflect.KFunction
import kotlin.reflect.full.declaredFunctions
import kotlin.reflect.full.findAnnotation

class ProtocolCodecEngine {

    val decoder = ProtocolDecoder()
    val encoder = ProtocolEncoder()

    lateinit var context: Any

    fun frameDeclare(templateStr: String) {
        val flags = ProtocolTemplateDecoder.parse(templateStr)
        decoder.setFlags(flags)
        encoder.setFlags(flags)
    }

    fun setVerifyFlags(flagStr: String) {
        val flags = ProtocolTemplateDecoder.parse(flagStr, false)
        decoder.setVerifyFlags(flags)
        encoder.setVerifyFlags(flags)
    }

    fun setTypeEncodeByteSize(size: Int) {
        decoder.typeByteSize = size
        encoder.typeByteSize = size
    }

    fun setSizeMaxValue(value: Int) {
        decoder.setSizeMaxValue(value)
    }

    fun setBufferMaxSize(size: Int) {
        decoder.bufferMaxSize = size
    }

    fun setDecodeTimeout(ms: Int) {
        decoder.decodeTimeout = ms
    }

    fun registerCallback(context: Any) {
        this.context = context
    }

    fun appendBuffer(buffer: ByteArray) {
        decoder.addBuffer(buffer)
    }

    inline fun<reified T : Any> registerType() {
        val kClass: KClass<T> = T::class
        val typeAnnotation = kClass.annotations.filterIsInstance<Type>().firstOrNull()
        if (typeAnnotation == null) {
            throw IllegalArgumentException("Class ${kClass.simpleName} must be annotated with @Type!")
        }
        val codec = when(typeAnnotation.codec) {
            CodecType.JSON -> JsonCodec(kClass)
            CodecType.Bytes -> BytesCodec(kClass)
        }
        val callback by lazy {
            findMethodByType(kClass)
        }
        decoder.typeDecoders[typeAnnotation.id] = { content ->
            val data = codec.decode(content)
            callback.call(context, data)
        }
        encoder.typeEncoders[typeAnnotation.id] = { data ->
            codec.encode(data as T)
        }
    }

    fun registerType(id: Int) {
        val callback = findMethodByType(id)
        decoder.typeDecoders[id] = { content ->
            callback.call(context)
        }
    }

    inline fun<reified T: Any> encode(data: T): ByteArray {
        val kClass: KClass<T> = T::class
        val typeAnnotation = kClass.annotations.filterIsInstance<Type>().firstOrNull()
        if (typeAnnotation == null) {
            throw IllegalStateException("Class ${kClass.simpleName} must be annotated with @Type!")
        }
        val contentEncoder = encoder.typeEncoders[typeAnnotation.id]
        val content = contentEncoder?.invoke(data)
            ?: throw IllegalStateException("Can not find encoder for ${kClass.simpleName}")
        return encoder.encodeFrame(content, typeAnnotation.id)
    }

    fun encodeId(id: Int): ByteArray {
        return encoder.encodeFrame(ByteArray(0), id)
    }

    fun findMethodByType(clazz: KClass<*>): KFunction<*> {
        if (!::context.isInitialized) {
            throw IllegalStateException("Context is not register!")
        }
        context::class.declaredFunctions.forEach { method ->
            val annotation = method.findAnnotation<Subscribe>()
            if (annotation != null) {
                method.parameters.forEach { parameter ->
                    if (parameter.type.classifier == clazz) {
                        return method
                    }
                }
            }
        }
        throw IllegalStateException("Can not find subscribe method for $clazz with param type: ${clazz.simpleName}")
    }

    fun findMethodByType(id: Int): KFunction<*> {
        if (!::context.isInitialized) {
            throw IllegalStateException("Context is not register!")
        }
        context::class.declaredFunctions.forEach { method ->
            val annotation = method.findAnnotation<Subscribe>()
            if (annotation?.id == id) {
                return method
            }
        }
        throw IllegalArgumentException("Can not find subscribe method for id: $id")
    }
}