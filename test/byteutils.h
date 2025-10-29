#pragma once

#include <qbytearray.h>
#include <qrandom.h>

struct TByteUtils {
    template<size_t N>
    static QByteArray make(const uchar(&a)[N]) {
        return { (const char*)a, N };
    }

    static QByteArray makeRandomBytes(int length) {
        QByteArray data;
        data.resize(length);
        for (int i = 0; i < length; ++i)
            data[i] = static_cast<char>(QRandomGenerator::global()->bounded(0, 256));
        return data;
    }
};