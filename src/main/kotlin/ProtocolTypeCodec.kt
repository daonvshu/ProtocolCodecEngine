package com.daonvshu.protocol.codec

import com.google.gson.Gson

enum class CodecType {
    JSON,
    Bytes,
}

interface ProtocolTypeCodec<T> {
    fun decode(content: ByteArray): T
    fun encode(content: T): ByteArray
}

class JsonCodec<T>(private val clazz: Class<T>) : ProtocolTypeCodec<T> {

    private val json = Gson()

    override fun decode(content: ByteArray): T {
        return json.fromJson<T>(content.toString(), clazz.javaClass)
    }

    override fun encode(content: T): ByteArray {
        return json.toJson(content).toByteArray()
    }
}

interface IBytesCodec {
    fun fromBytes(bytes: ByteArray)
    fun toBytes(): ByteArray
}

class BytesCodec<T>(private val clazz: Class<T>) : ProtocolTypeCodec<T> {
    override fun decode(content: ByteArray): T {
        if (clazz.javaClass != (IBytesCodec::class as Any).javaClass) {
            throw IllegalArgumentException("Class ${clazz.simpleName} must implement IBytesCodec!")
        }
        return clazz.getDeclaredConstructor().newInstance().apply { (this as IBytesCodec).fromBytes(content) }
    }

    override fun encode(content: T): ByteArray {
        if (clazz.javaClass != (IBytesCodec::class as Any).javaClass) {
            throw IllegalArgumentException("Class ${clazz.simpleName} must implement IBytesCodec!")
        }
        return (content as IBytesCodec).toBytes()
    }
}