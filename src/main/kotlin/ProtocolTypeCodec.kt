package com.daonvshu.protocol.codec

import com.google.gson.Gson
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

    private val json = Gson()

    override fun decode(content: ByteArray): T {
        val jsonStr = content.toString(Charsets.UTF_8)  // 使用 UTF-8 解码字节
        return json.fromJson(jsonStr, clazz.java)
    }

    override fun encode(content: T): ByteArray {
        return json.toJson(content).toByteArray(Charsets.UTF_8)
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