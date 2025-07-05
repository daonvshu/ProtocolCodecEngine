package com.daonvshu.protocol.codec

import com.daonvshu.protocol.codec.annotations.Subscribe
import com.daonvshu.protocol.codec.annotations.Type
import java.io.File
import java.io.FileInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

data class AdcData(
    val adc: ByteArray = ByteArray(12),
    val turbine: UInt = 20u
)

@Type(id = 0, codec = CodecType.Bytes)
data class FrameData(
    val data: Array<AdcData> = Array(ADC_COUNT) { AdcData() }
): IBytesCodec {
    companion object {
        const val ADC_SIZE = 16
        const val ADC_COUNT = 2048
    }

    override fun fromBytes(bytes: ByteArray) {
        require(bytes.size >= ADC_SIZE * ADC_COUNT) { "Byte array too short" }
        val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
        for (i in 0 until ADC_COUNT) {
            val adcBytes = ByteArray(12)
            buffer.get(adcBytes)  // 复制12字节adc数据
            val turbine = buffer.int.toUInt()  // 读取4字节整型
            data[i] = AdcData(adcBytes, turbine)
        }
    }

    override fun toBytes(): ByteArray {
        val result = ByteArray(ADC_SIZE * ADC_COUNT)
        val buffer = ByteBuffer.wrap(result).order(ByteOrder.LITTLE_ENDIAN)
        for (i in 0 until ADC_COUNT) {
            buffer.put(data[i].adc)
            buffer.putInt(data[i].turbine.toInt())
        }
        return result
    }
}

class DecodeTest {
    private val codec = ProtocolCodecEngine()
    private var decodedObjSize = 0

    init {
        codec.frameDeclare("H(5AFF)S2CV(SUM16)E(FE)")
        codec.setVerifyFlags("SC")

        codec.registerCallback(this)
        codec.registerType<FrameData>()
    }

    fun runTest() {
        //按行读取1.bin，每次读取102400个字节
        val file = File("1.bin")
        if (!file.exists()) {
            println("文件不存在")
            return
        }

        val bufferSize = 102400
        val startTime = System.nanoTime() // 记录开始时间
        FileInputStream(file).use { inputStream ->
            val buffer = ByteArray(bufferSize)
            var bytesRead: Int
            while (inputStream.read(buffer).also { bytesRead = it } > 0) {
                // 只传递实际读取的字节数
                val data = if (bytesRead == bufferSize) buffer else buffer.copyOf(bytesRead)
                codec.appendBuffer(data)
            }
        }

        val endTime = System.nanoTime() // 记录结束时间
        val durationMs = (endTime - startTime) / 1_000_000 // 毫秒
        println("执行耗时: $durationMs 毫秒，解码对象个数：$decodedObjSize")
    }

    @Subscribe
    fun onFrameDataCallback(frameData: FrameData) {
        decodedObjSize++
    }
}

fun main() {
    DecodeTest().runTest()
}