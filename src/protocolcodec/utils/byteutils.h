#pragma once

#include <protocolcodec/global.h>

#include <qbytearray.h>

PROTOCOL_CODEC_NAMESPACE_BEGIN

struct PROTOCOL_CODEC_EXPORT ByteUtils {
    template<typename T>
    static void byteSwap(T& data, size_t size) {
        byteSwap((char*)&data, size);
    }

    static void byteSwap(char* data, size_t size) {
        for (size_t i = 0; i < size / 2; i++) {
            char* pl = data + i;
            char* pr = data + (size - i - 1);
            if (*pl != *pr) {
                *pl ^= *pr;
                *pr ^= *pl;
                *pl ^= *pr;
            }
        }
    }

    static void byteSwap(QByteArray& bytes) {
        std::reverse(bytes.begin(), bytes.end());
    }
};

PROTOCOL_CODEC_NAMESPACE_END