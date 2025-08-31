package com.daonvshu.protocol.codec

import kotlinx.serialization.KSerializer
import kotlinx.serialization.json.Json
import kotlinx.serialization.serializer
import kotlin.reflect.KClass
import kotlin.reflect.full.createInstance
import kotlin.reflect.full.isSubclassOf

enum class CodecType {
    JSON,
    Bytes,
}

interface ProtocolTypeCodec<T> {
    fun decode(content: ByteArray): T
    fun encode(content: T): ByteArray
}

class JsonCodec<T: Any>(private val clazz: KClass<T>) : ProtocolTypeCodec<T> {

    private val json = Json {
        ignoreUnknownKeys = true
        encodeDefaults = true
        isLenient = true
    }

    @Suppress("UNCHECKED_CAST")
    private val serializer: KSerializer<T> by lazy {
        serializer(clazz.java) as KSerializer<T>
    }

    override fun decode(content: ByteArray): T {
        val jsonStr = content.toString(Charsets.UTF_8)
        return json.decodeFromString(serializer, jsonStr)
    }

    override fun encode(content: T): ByteArray {
        val jsonStr = json.encodeToString(serializer, content)
        return jsonStr.toByteArray(Charsets.UTF_8)
    }
}

interface IBytesCodec {
    fun fromBytes(bytes: ByteArray)
    fun toBytes(): ByteArray
}

class BytesCodec<T: Any>(private val clazz: KClass<T>) : ProtocolTypeCodec<T> {
    private fun checkValid() {
        if (!clazz.isSubclassOf(IBytesCodec::class)) {
            throw IllegalArgumentException("Class ${clazz.simpleName} must implement IBytesCodec!")
        }
    }

    override fun decode(content: ByteArray): T {
        checkValid()
        // 反射创建实例（clazz 需有无参构造）
        val instance = clazz.createInstance()
        (instance as IBytesCodec).fromBytes(content)
        return instance
    }

    override fun encode(content: T): ByteArray {
        checkValid()
        return (content as IBytesCodec).toBytes()
    }
}