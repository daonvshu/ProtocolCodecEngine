#include "layoutmatrix.h"
#include "adapter.h"

#include <QJsonArray>
#include <QMap>
#include <QStringList>
#include <algorithm>
#include <stdexcept>

namespace {
void check(bool condition, const QString& message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}

QByteArray numberBytes(int value, int width, bool bigEndian) {
    QByteArray result;
    for (int i = 0; i < width; ++i) {
        const int shift = (bigEndian ? width - i - 1 : i) * 8;
        result.append(char(quint32(value) >> shift));
    }
    return result;
}

QByteArray independentCrc(const QByteArray& bytes, bool bigEndian) {
    quint16 value = 0xffff;
    for (unsigned char byte : bytes) {
        value ^= byte;
        for (int bit = 0; bit < 8; ++bit) value = (value >> 1) ^ ((value & 1) ? 0xa001 : 0);
    }
    return numberBytes(value, 2, bigEndian);
}

struct Layout {
    QString mode;
    QString order;
    bool bigEndian;

    int width() const { return bigEndian ? 2 : 1; }

    DecoderConfig config(bool metadata, bool logging) const {
        DecoderConfig result;
        result.format.clear();
        result.verifyFlags.clear();
        result.sizeFlags = "TC";
        result.typeLength = mode == "type";
        result.metadata = metadata;
        result.logging = logging;
        for (const QChar field : order) {
            const QString endian = bigEndian ? "!" : "";
            if (field == 'H') result.format += "H(5AFA)";
            else if (field == 'E') result.format += "E(FEEF)";
            else if (field == 'V') result.format += "V" + endian + "(CRC16)";
            else if (field == 'C') result.format += mode == "fixed" ? "C8" : "C";
            else result.format += QString(field) + QString::number(width()) + endian;
            if (field != 'V' && order.contains('V') && order.indexOf(field) < order.indexOf('V')) {
                result.verifyFlags += field;
            }
        }
        return result;
    }

    QByteArray frame(const QByteArray& payload, int type, QByteArray* metadata) const {
        QMap<QChar, QByteArray> fields;
        fields['H'] = QByteArray::fromHex("5afa");
        fields['E'] = QByteArray::fromHex("feef");
        fields['A'] = numberBytes(10, width(), bigEndian);
        fields['T'] = numberBytes(type, width(), bigEndian);
        fields['S'] = numberBytes(width() + payload.size(), width(), bigEndian);
        fields['C'] = payload;
        QByteArray result;
        for (const QChar field : order) {
            if (field == 'V') fields['V'] = independentCrc(result, bigEndian);
            result += fields[field];
        }
        if (metadata != nullptr) {
            metadata->clear();
            for (const QChar field : QString("HTASCVEV")) {
                if (order.contains(field)) *metadata += fields[field];
            }
        }
        return result;
    }

    QString baselineExclusion() const {
        // The original extraction loop skips the last node, including Type or Content.
        if (order.endsWith('T') || order.endsWith('C')) return "original-terminal-field-extraction";
        // Avoid invoking the proven infinite loop inside the shared comparison process.
        if (mode == "type" && order.mid(order.indexOf('T') + 1,
            order.indexOf('C') - order.indexOf('T') - 1).contains('A')) {
            return "original-type-length-loop";
        }
        return {};
    }
};

struct Counts {
    quint64 partitions = 0;
    quint64 feeds = 0;
    quint64 baselineChecks = 0;
    quint64 recoveryChecks = 0;
};

void testLayout(const Layout& layout, Counts& counts) {
    const bool hasContent = layout.order.contains('C');
    const QByteArray first = hasContent ? QByteArray::fromHex("3412000078560000") : QByteArray();
    const QByteArray second = hasContent && layout.mode != "size"
        ? QByteArray::fromHex("4422000088660000") : QByteArray();
    const auto firstFrame = layout.frame(first, 100, nullptr);
    const auto secondFrame = layout.frame(second, 101, nullptr);
    const auto stream = firstFrame + secondFrame;
    for (const bool metadata : {false, true}) {
        const auto config = layout.config(metadata, false);
        QByteArray firstMeta, secondMeta;
        layout.frame(first, 100, &firstMeta);
        layout.frame(second, 101, &secondMeta);
        const QList<DecodedPacket> expected = {
            {100, first, metadata ? firstMeta : QByteArray()},
            {101, second, metadata ? secondMeta : QByteArray()}};
        auto baseline = makeBaseline(config, [](const DecodedPacket&) {});
        auto current = makeCurrent(config, [](const DecodedPacket&) {});
        check(baseline->encode(first, 100) == firstFrame, "original encoding differs from independent bytes");
        check(current->encode(first, 100) == firstFrame, "current encoding differs from independent bytes");
        check(baseline->encode(second, 101) == secondFrame, "original second encoding differs");
        check(current->encode(second, 101) == secondFrame, "current second encoding differs");
        if (layout.baselineExclusion().isEmpty()) {
            QList<DecodedPacket> decoded;
            auto original = makeBaseline(config, [&](const DecodedPacket& packet) { decoded.append(packet); });
            original->feed(stream);
            check(decoded == expected, "original unsplit decoding differs from independent packets");
            ++counts.baselineChecks;
        }
        const auto partition = [&](const QList<int>& chunks, bool logging) {
            QList<DecodedPacket> decoded;
            auto decoder = makeCurrent(layout.config(metadata, logging),
                [&](const DecodedPacket& packet) { decoded.append(packet); });
            int offset = 0;
            for (const int bytes : chunks) {
                decoder->feed(stream.mid(offset, bytes));
                offset += bytes;
                ++counts.feeds;
                const int completed = int(offset >= firstFrame.size()) + int(offset >= stream.size());
                check(decoded == expected.mid(0, completed),
                    QString("partition callback mismatch at byte %1 metadata=%2 logging=%3")
                        .arg(offset).arg(metadata).arg(logging));
            }
            check(offset == stream.size() && decoded == expected, "partition did not deliver both packets");
            ++counts.partitions;
        };
        for (int a = 0; a <= stream.size(); ++a) {
            for (int b = a; b <= stream.size(); ++b) {
                partition({a, b - a, stream.size() - b}, false);
            }
        }
        QList<int> singleBytes;
        for (int i = 0; i < stream.size(); ++i) singleBytes.append(1);
        partition(singleBytes, false);
        partition(singleBytes, true);
        if (layout.order.startsWith('H') && layout.order.contains('V')) {
            auto damaged = firstFrame;
            int verifyOffset = 0;
            for (const QChar field : layout.order) {
                if (field == 'V') break;
                verifyOffset += field == 'C' ? first.size()
                    : field == 'H' || field == 'E' ? 2 : layout.width();
            }
            damaged[verifyOffset] = char(uchar(damaged.at(verifyOffset)) ^ 1);
            QList<DecodedPacket> decoded;
            auto decoder = makeCurrent(layout.config(metadata, true),
                [&](const DecodedPacket& packet) { decoded.append(packet); });
            decoder->feed(QByteArray::fromHex("010203") + damaged + secondFrame);
            check(decoded == expected.mid(1), "CRC/noise recovery callback mismatch");
            ++counts.recoveryChecks;
        }
    }
}
}

QJsonObject runLayoutMatrix() {
    QList<Layout> layouts;
    for (const auto& mode : {QString("size"), QString("fixed"), QString("type"), QString("command")}) {
        for (int mask = 0; mask < 16; ++mask) {
            QString core = "T";
            if (mask & 1) core += 'A';
            if (mode == "size") core += "SC";
            else if (mode != "command") core += 'C';
            std::sort(core.begin(), core.end());
            do {
                if (mode == "size" && core.indexOf('S') > core.indexOf('C')) continue;
                if (mode == "type" && core.indexOf('T') > core.indexOf('C')) continue;
                const QString prefix = mask & 2 ? "H" : "";
                const QString suffix = QString(mask & 4 ? "V" : "") + QString(mask & 8 ? "E" : "");
                for (const bool bigEndian : {false, true}) {
                    layouts.append({mode, prefix + core + suffix, bigEndian});
                    if ((mask & 12) == 12) layouts.append({mode, prefix + core + "EV", bigEndian});
                }
            } while (std::next_permutation(core.begin(), core.end()));
        }
    }
    // Size is optional for commands without Content, including a trailing Size field.
    QString commandCore = "AST";
    do {
        layouts.append({"command", "H" + commandCore + "VE", false});
        layouts.append({"command", "H" + commandCore + "EV", true});
    } while (std::next_permutation(commandCore.begin(), commandCore.end()));
    layouts.append({"command", "HATVES", false});
    layouts.append({"command", "HATVES", true});
    // A constant header inside a frame uses general scanning instead of the leading-header fast path.
    layouts.append({"size", "STCHVE", false});
    layouts.append({"fixed", "CHTVE", true});
    layouts.append({"type", "TCHVE", false});

    Counts counts;
    QJsonArray results, failures;
    for (const auto& layout : layouts) {
        QJsonObject result{{"mode", layout.mode}, {"format", layout.config(false, false).format},
            {"verify_flags", layout.config(false, false).verifyFlags},
            {"baseline_decode_exclusion", layout.baselineExclusion()}};
        try {
            testLayout(layout, counts);
            result.insert("passed", true);
        } catch (const std::exception& error) {
            result.insert("passed", false);
            result.insert("failure", QString::fromUtf8(error.what()));
            failures.append(result);
        }
        results.append(result);
    }
    return {{"passed", failures.isEmpty()}, {"layouts", layouts.size()},
        {"partition_cases", double(counts.partitions)}, {"feed_calls", double(counts.feeds)},
        {"baseline_unsplit_checks", double(counts.baselineChecks)},
        {"noise_and_bad_crc_recovery_checks", double(counts.recoveryChecks)},
        {"baseline_commit", COMPARISON_BASELINE}, {"results", results}, {"failures", failures}};
}
