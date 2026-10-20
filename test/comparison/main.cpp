#include "adapter.h"
#include "layoutmatrix.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QTextStream>
#include <QThread>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr quint64 InitialHash = 1469598103934665603ULL;
QFile* diagnosticFile = nullptr;
quint64 diagnosticCount = 0;
quint64 diagnosticBytes = 0;

void require(bool condition, const QString& description) {
    if (!condition) throw std::runtime_error(description.toStdString());
}

void progress(const QString& message) {
    QTextStream out(stdout);
    out << message << '\n';
    out.flush();
}

void messageHandler(QtMsgType, const QMessageLogContext&, const QString& message) {
    ++diagnosticCount;
    const auto bytes = message.toUtf8();
    diagnosticBytes += quint64(bytes.size()) + 1;
    if (diagnosticFile != nullptr) {
        if (diagnosticFile->write(bytes) != bytes.size() || diagnosticFile->write("\n", 1) != 1) {
            std::abort();
        }
    }
}

void writeJson(const QString& path, const QJsonObject& object) {
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "cannot create test result");
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
    require(file.write(bytes) == bytes.size(), "cannot write test result");
}

struct Random {
    quint32 state = 0x63e27a91;
    quint32 next() { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
    int bounded(int limit) { return int(next() % quint32(limit)); }
    QByteArray bytes(int size) {
        QByteArray result(size, char(0));
        for (auto& byte : result) byte = char(next());
        return result;
    }
};

QList<int> chunksFor(int size, int mode) {
    QList<int> chunks;
    Random random;
    int offset = 0;
    while (offset < size) {
        const int chunk = mode == 0 ? size : mode == 1 ? 1 : mode == 2 ? 7
            : mode == 3 ? 31 : random.bounded(23) + 1;
        chunks.append(qMin(chunk, size - offset));
        offset += chunks.last();
    }
    return chunks;
}

QByteArray crc16(const QByteArray& bytes) {
    quint16 crc = 0xffff;
    for (unsigned char byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
    }
    QByteArray result;
    result.append(char(crc));
    result.append(char(crc >> 8));
    return result;
}

struct Suite {
    quint64 cases = 0;
    quint64 feedCalls = 0;
    quint64 baselineSplitLossCases = 0;
    QJsonArray splitLossExamples;
    QJsonArray groups;

    void compare(const QString& label, const DecoderConfig& config, const QByteArray& stream,
                 const QList<int>& chunks, int expectedCount = -1) {
        QList<DecodedPacket> baselinePackets, currentPackets;
        QList<DecodedPacket> oraclePackets, baselineOracle;
        auto oracle = makeCurrent(config, [&](const DecodedPacket& packet) { oraclePackets.append(packet); });
        auto oldOracle = makeBaseline(config, [&](const DecodedPacket& packet) { baselineOracle.append(packet); });
        oracle->feed(stream);
        oldOracle->feed(stream);
        require(oraclePackets == baselineOracle, label + ": unsplit baseline mismatch");
        auto baseline = makeBaseline(config, [&](const DecodedPacket& packet) { baselinePackets.append(packet); });
        auto current = makeCurrent(config, [&](const DecodedPacket& packet) { currentPackets.append(packet); });
        int offset = 0;
        bool baselineLostPacket = false;
        for (const int size : chunks) {
            require(size >= 0 && offset + size <= stream.size(), label + ": invalid test partition");
            const auto part = stream.mid(offset, size);
            baseline->feed(part);
            current->feed(part);
            ++feedCalls;
            if (!(baselinePackets == currentPackets)) {
                // The pinned baseline drops a valid header on a split Size field.
                // Accept only lost baseline callbacks: the current stream must match
                // the baseline's own unsplit oracle, including all payload and metadata bytes.
                bool subsequence = baselinePackets.size() < currentPackets.size();
                int match = 0;
                for (const auto& packet : currentPackets) {
                    if (match < baselinePackets.size() && packet == baselinePackets[match]) ++match;
                }
                subsequence = subsequence && match == baselinePackets.size();
                const bool oraclePrefix = currentPackets == oraclePackets.mid(0, currentPackets.size());
                const bool nestedHeaderLoss = label == "nested-valid-frame-split4"
                    || label == "nested-valid-frame-split5";
                require(oraclePrefix && (subsequence || nestedHeaderLoss),
                    label + QString(": callback mismatch at input byte %1 baseline=%2 current=%3")
                        .arg(offset + size).arg(baselinePackets.size()).arg(currentPackets.size()));
                baselineLostPacket = true;
            }
            offset += size;
        }
        require(offset == stream.size(), label + ": incomplete test partition");
        if (expectedCount >= 0) require(currentPackets.size() == expectedCount, label + ": expected callback count differs");
        if (baselineLostPacket) {
            require(currentPackets == oraclePackets, label + ": fixed result differs from unsplit oracle");
            ++baselineSplitLossCases;
            if (splitLossExamples.size() < 5) splitLossExamples.append(label);
        }
        ++cases;
    }

    QByteArray encoded(const DecoderConfig& config, const QByteArray& payload, int type = 100) {
        auto baseline = makeBaseline(config, [](const DecodedPacket&) {});
        auto current = makeCurrent(config, [](const DecodedPacket&) {});
        const auto frame = baseline->encode(payload, type);
        require(frame == current->encode(payload, type), "encoding differs from baseline");
        return frame;
    }

    void group(const QString& name, quint64 start) {
        groups.append(QJsonObject{{"name", name}, {"cases", double(cases - start)}});
        progress(QString("CORRECTNESS %1 cases=%2").arg(name).arg(cases - start));
    }
};

void verifyKnownBug(const QString& outputDirectory, const QString& name) {
    QJsonObject outcomes;
    for (const auto& version : {QString("baseline"), QString("current")}) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QCoreApplication::applicationFilePath(), {"--probe", name, version});
        require(process.waitForStarted(3000), "cannot start isolated baseline probe");
        const bool finished = process.waitForFinished(1500);
        if (!finished) { process.kill(); process.waitForFinished(3000); }
        const auto result = QString::fromUtf8(process.readAll());
        outcomes.insert(version, QJsonObject{{"completed", finished},
            {"normal_exit", finished && process.exitStatus() == QProcess::NormalExit},
            {"exit_code", process.exitCode()}, {"output", result.trimmed()}});
        if (version == "current") require(finished && process.exitCode() == 0, "fixed probe failed: " + name);
        else require(!finished || process.exitCode() != 0, "baseline bug was not reproduced: " + name);
    }
    writeJson(outputDirectory + "/bug-" + name + ".json", outcomes);
    progress("KNOWN_BUG " + name + " baseline reproduced; current passed");
}

int probe(const QString& name, bool baseline) {
    DecoderConfig config;
    QByteArray frame, expected;
    if (name == "type-length-loop") {
        config.format = "H(5AFA)T1A1CE(FEEF)";
        config.typeLength = true;
        expected = QByteArray::fromHex("3412000078560000");
        frame = QByteArray::fromHex("5afa640a") + expected + QByteArray::fromHex("feef");
    } else if (name == "content-last") {
        config.format = "H(5AFA)T1S2C";
        expected = QByteArray::fromHex("3412000078560000");
        frame = QByteArray::fromHex("5afa640900") + expected;
    } else if (name == "zero-size") {
        config.format = "H(5AFA)T1S1CV(CRC16)E(FE)";
        config.verifyFlags = "STC";
        frame = QByteArray::fromHex("5afa6400");
        frame += crc16(frame.mid(2));
        frame.append(char(0xfe));
    } else if (name == "split-size") {
        frame = QByteArray::fromHex("5afa640a090034120000785600002cecfeef");
        expected = QByteArray::fromHex("3412000078560000");
    } else {
        return 3;
    }
    QList<DecodedPacket> packets;
    auto callback = [&](const DecodedPacket& packet) { packets.append(packet); };
    auto decoder = baseline ? makeBaseline(config, callback) : makeCurrent(config, callback);
    if (name == "split-size") {
        decoder->feed(frame.left(4));
        decoder->feed(frame.mid(4) + frame);
    } else {
        decoder->feed(frame);
    }
    const bool valid = name == "zero-size" ? packets.isEmpty()
        : packets.size() == (name == "split-size" ? 2 : 1) && packets.first().content == expected;
    progress(QString("probe=%1 callbacks=%2 expected_behavior=%3").arg(name).arg(packets.size()).arg(valid));
    return valid ? 0 : 1;
}

QJsonObject correctness(const QString& directory) {
    Suite suite;
    quint64 start = suite.cases;
    QList<DecoderConfig> formats;
    formats.append(DecoderConfig{});
    auto config = DecoderConfig{};
    config.format = "H(5AFA)T2!A2!S2!CV!(CRC16)E(FEEF)";
    formats.append(config);
    config = DecoderConfig{};
    config.format = "H(FAFA)S1T2CV(CRC16)E(EF)";
    config.verifyFlags = "HSTC";
    config.maximumSize = 255;
    formats.append(config);
    for (const auto& verify : {QString("SUM8"), QString("SUM16")}) {
        config = DecoderConfig{};
        config.format = "H(5AFA)T1A1S2CV(" + verify + ")E(FEEF)";
        formats.append(config);
    }
    config = DecoderConfig{};
    config.format = "H(5AFA)T1A1C8E(FEEF)";
    formats.append(config);
    config = DecoderConfig{};
    config.format = "H(5AFA)T1A1S2CE(FEEF)";
    formats.append(config);
    config = DecoderConfig{};
    config.format = "H(5A5A5A)T1S2CV(CRC16)E(FEEF)";
    config.verifyFlags = "STC";
    formats.append(config);

    for (int index = 0; index < formats.size(); ++index) {
        for (const bool metadata : {false, true}) {
            config = formats[index];
            config.metadata = metadata;
            const auto frame = suite.encoded(config, QByteArray::fromHex("3412000078560000"));
            const auto next = suite.encoded(config, QByteArray::fromHex("4422000088660000"), 101);
            const auto stream = frame + next;
            for (int a = 0; a <= stream.size(); ++a) {
                for (int b = a; b <= stream.size(); ++b) {
                    suite.compare(QString("partition-format%1-meta%2-a%3-b%4").arg(index).arg(metadata).arg(a).arg(b),
                        config, stream, {a, b - a, stream.size() - b}, 2);
                }
            }
            suite.compare("empty-feeds", config, stream, {0, 0, stream.size(), 0}, 2);
        }
    }
    suite.group("eight-protocols-all-two-and-three-way-splits-and-metadata", start);

    start = suite.cases;
    config = formats[2];
    const auto command = suite.encoded(config, QByteArray());
    require(command.size() < 20, "unexpected command length");
    for (quint32 mask = 0; mask < (quint32(1) << (command.size() - 1)); ++mask) {
        QList<int> chunks;
        int begin = 0;
        for (int end = 1; end < command.size(); ++end) {
            if (mask & (quint32(1) << (end - 1))) { chunks.append(end - begin); begin = end; }
        }
        chunks.append(command.size() - begin);
        suite.compare("all-command-partitions", config, command, chunks, 1);
    }
    suite.group("all-command-partition-combinations", start);

    start = suite.cases;
    for (int index = 0; index < 5; ++index) {
        config = formats[index];
        const auto frame = suite.encoded(config, QByteArray::fromHex("3412000078560000"));
        const auto good = suite.encoded(config, QByteArray::fromHex("4422000088660000"), 101);
        QList<QByteArray> mutations;
        for (int byte = 0; byte < frame.size(); ++byte) {
            for (int bit = 0; bit < 8; ++bit) {
                auto altered = frame;
                altered[byte] = char(uchar(altered.at(byte)) ^ (1 << bit));
                mutations.append(altered);
            }
            auto altered = frame;
            altered.remove(byte, 1);
            mutations.append(altered);
        }
        for (int byte = 0; byte <= frame.size(); ++byte) {
            for (const auto value : {0x00, 0x5a, 0xfa, 0xfe, 0xef}) {
                auto altered = frame;
                altered.insert(byte, char(value));
                mutations.append(altered);
            }
        }
        for (int mutation = 0; mutation < mutations.size(); ++mutation) {
            const auto stream = mutations[mutation] + good + frame;
            for (int mode = 0; mode < 5; ++mode) {
                suite.compare(QString("mutation-format%1-case%2-mode%3").arg(index).arg(mutation).arg(mode),
                              config, stream, chunksFor(stream.size(), mode));
            }
        }
    }
    suite.group("single-bit-flips-deletions-insertions-and-resynchronization", start);

    start = suite.cases;
    Random random;
    config = formats.first();
    for (int sample = 0; sample < 1000; ++sample) {
        QByteArray stream;
        for (int item = 0; item < 12; ++item) {
            stream += random.bytes(random.bounded(32));
            auto frame = suite.encoded(config, random.bytes(random.bounded(129)), item % 2 == 0 ? 100 : 101);
            if (item % 3 == 0) frame[frame.size() - 3] = char(frame.at(frame.size() - 3) ^ 1);
            stream += frame;
        }
        for (int mode = 0; mode < 5; ++mode) {
            suite.compare(QString("random-seed-fixed-%1-mode%2").arg(sample).arg(mode),
                          config, stream, chunksFor(stream.size(), mode));
        }
    }
    suite.group("deterministic-random-streams-and-chunks", start);

    start = suite.cases;
    for (const int limit : {16, 17, 18, 19, 32, 64, 65536}) {
        config = formats.first();
        config.bufferLimit = limit;
        config.maximumSize = 32;
        const auto frame = suite.encoded(config, QByteArray(8, 'a'));
        auto oversized = frame;
        oversized[4] = char(255);
        const auto stream = QByteArray(100, '\x5a') + oversized + frame + frame;
        for (int mode = 0; mode < 5; ++mode) suite.compare("cache-boundary", config, stream, chunksFor(stream.size(), mode));
    }
    config = formats.first();
    const auto inner = suite.encoded(config, QByteArray(8, 'b'));
    const auto outer = suite.encoded(config, QByteArray(3, 'a') + inner + QByteArray(8, 'c'));
    for (int split = 1; split < outer.size(); ++split) {
        suite.compare(QString("nested-valid-frame-split%1").arg(split), config, outer,
                      {split, outer.size() - split});
    }
    suite.group("cache-limits-overlapping-header-and-nested-frame-policy", start);

    start = suite.cases;
    for (int sample = 0; sample < 100; ++sample) {
        config = formats.first();
        config.logging = true;
        const auto frame = suite.encoded(DecoderConfig{}, random.bytes(32));
        auto bad = frame;
        bad[bad.size() - 1] = char(0);
        suite.compare("logging-does-not-change-callbacks", config, bad + frame, chunksFor(2 * frame.size(), 4), 1);
    }
    suite.group("logging-enabled-comparison", start);
    start = suite.cases;
    for (const bool expire : {false, true}) {
        config = DecoderConfig{};
        config.timeout = 50;
        const auto frame = suite.encoded(config, QByteArray(8, 'x'));
        QList<DecodedPacket> baselinePackets, currentPackets;
        auto baseline = makeBaseline(config, [&](const DecodedPacket& packet) { baselinePackets.append(packet); });
        auto current = makeCurrent(config, [&](const DecodedPacket& packet) { currentPackets.append(packet); });
        baseline->feed(frame.left(10));
        current->feed(frame.left(10));
        if (expire) QThread::msleep(80);
        baseline->feed(frame.mid(10));
        current->feed(frame.mid(10));
        require(baselinePackets == currentPackets, "timeout callback mismatch");
        require(currentPackets.size() == (expire ? 0 : 1), "timeout expected callback differs");
        baseline->clear();
        current->clear();
        baseline->feed(frame);
        current->feed(frame);
        require(baselinePackets == currentPackets, "clear-and-restart callback mismatch");
        ++suite.cases;
    }
    suite.group("receive-gap-timeout-and-explicit-clear", start);
    for (const auto& bug : {QString("type-length-loop"), QString("content-last"), QString("zero-size"), QString("split-size")}) {
        verifyKnownBug(directory, bug);
    }
    return {{"baseline_commit", COMPARISON_BASELINE}, {"passed", true},
            {"comparison_cases", double(suite.cases)}, {"feed_calls_per_version", double(suite.feedCalls)},
            {"groups", suite.groups}, {"known_bug_probes", 4},
            {"baseline_split_loss_cases", double(suite.baselineSplitLossCases)},
            {"split_loss_examples", suite.splitLossExamples}};
}

struct Digest {
    quint64 count = 0;
    quint64 hash = InitialHash;
    void receive(const DecodedPacket& packet) {
        ++count;
        hash = (hash ^ quint64(packet.type)) * 1099511628211ULL;
        hash = (hash ^ quint64(packet.content.size())) * 1099511628211ULL;
        for (unsigned char byte : packet.content) hash = (hash ^ byte) * 1099511628211ULL;
    }
};

struct Timing {
    double milliseconds;
    double maximumFeedMs;
    Digest digest;
    quint64 messages;
    quint64 logBytes;
};

struct Corpus {
    QByteArray bytes;
    QVector<int> recordEnds;
};

Timing timed(bool baseline, const Corpus& corpus, int bytes, int chunk,
             bool logging, const QString& logPath) {
    DecoderConfig config;
    config.logging = logging;
    config.maximumSize = 60000;
    Digest digest;
    auto callback = [&](const DecodedPacket& packet) { digest.receive(packet); };
    auto decoder = baseline ? makeBaseline(config, callback) : makeCurrent(config, callback);
    QFile log(logPath);
    if (logging) {
        require(log.open(QIODevice::WriteOnly), "cannot open benchmark diagnostic output");
        diagnosticFile = &log;
    }
    diagnosticCount = diagnosticBytes = 0;
    // Materialize input chunks outside the timed section for identical allocation costs.
    QList<QByteArray> parts;
    for (int offset = 0; offset < bytes;) {
        const int requestedEnd = qMin(offset + chunk, bytes);
        const auto end = std::lower_bound(corpus.recordEnds.cbegin(), corpus.recordEnds.cend(), requestedEnd);
        const int actualEnd = end == corpus.recordEnds.cend() ? bytes : qMin(*end, bytes);
        parts.append(corpus.bytes.mid(offset, actualEnd - offset));
        offset = actualEnd;
    }
    QElapsedTimer total, call;
    total.start();
    qint64 maxNs = 0;
    for (const auto& part : parts) {
        call.start();
        decoder->feed(part);
        maxNs = qMax(maxNs, call.nsecsElapsed());
    }
    if (logging) require(log.flush(), "cannot flush benchmark logs");
    const auto elapsed = total.nsecsElapsed();
    diagnosticFile = nullptr;
    return {double(elapsed) / 1e6, double(maxNs) / 1e6, digest, diagnosticCount, diagnosticBytes};
}

Corpus generateCorpus(const QString& path, bool mixed, int payloadSize) {
    const int minimum = 100 * 1024 * 1024;
    Corpus corpus;
    auto& data = corpus.bytes;
    data.reserve(minimum + 65536);
    Random random;
    auto encoder = makeCurrent(DecoderConfig{}, [](const DecodedPacket&) {});
    for (int index = 0; data.size() < minimum; ++index) {
        const auto payload = random.bytes(payloadSize);
        auto frame = encoder->encode(payload, index % 2 == 0 ? 100 : 101);
        if (mixed && index % 32 == 0) {
            switch ((index / 32) % 4) {
                case 0: frame[frame.size() - 4] = char(frame.at(frame.size() - 4) ^ 1); break;
                case 1: frame[frame.size() - 1] = char(0); break;
                case 2: frame[3] = char(11); break;
                case 3: frame[4] = char(0xff); frame[5] = char(0xff); break;
            }
            data += QByteArray::fromHex("01020304050607");
        }
        data += frame;
        corpus.recordEnds.append(data.size());
    }
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "cannot create generated corpus");
    require(file.write(data) == data.size(), "cannot save generated corpus");
    return corpus;
}

QJsonObject performance(const QString& directory) {
    QJsonArray results, corpora;
    bool passed = true;
    for (const bool mixed : {false, true}) {
        const auto label = mixed ? QString("mixed") : QString("valid");
        progress("GENERATING " + label + " >=100MiB");
        const auto corpus = generateCorpus(directory + "/" + label + ".bin", mixed, 64);
        const auto& data = corpus.bytes;
        corpora.append(QJsonObject{{"name", label}, {"bytes", data.size()},
            {"sha256", QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex())}});
        for (const int chunk : {64, 65536}) {
            QList<double> baselineMs, currentMs;
            double maximumMs = 0;
            Digest expected;
            for (int round = 0; round < 3; ++round) {
                Timing baseline, current;
                if (round % 2 == 0) {
                    baseline = timed(true, corpus, data.size(), chunk, false, {});
                    current = timed(false, corpus, data.size(), chunk, false, {});
                } else {
                    current = timed(false, corpus, data.size(), chunk, false, {});
                    baseline = timed(true, corpus, data.size(), chunk, false, {});
                }
                require(baseline.digest.count == current.digest.count && baseline.digest.hash == current.digest.hash,
                        "100MiB callback digest differs: " + label);
                baselineMs.append(baseline.milliseconds);
                currentMs.append(current.milliseconds);
                maximumMs = qMax(maximumMs, current.maximumFeedMs);
                expected = current.digest;
                progress(QString("TIMING %1 chunk=%2 round=%3 baseline_ms=%4 current_ms=%5")
                    .arg(label).arg(chunk).arg(round).arg(baseline.milliseconds).arg(current.milliseconds));
            }
            std::sort(baselineMs.begin(), baselineMs.end());
            std::sort(currentMs.begin(), currentMs.end());
            const double ratio = currentMs[1] / baselineMs[1];
            passed = passed && ratio <= 1.15;
            results.append(QJsonObject{{"corpus", label}, {"chunk_bytes", chunk}, {"bytes", data.size()},
                {"baseline_median_ms", baselineMs[1]}, {"current_median_ms", currentMs[1]},
                {"ratio", ratio}, {"frame_boundary_aligned", true}, {"current_maximum_feed_ms", maximumMs},
                {"callbacks", double(expected.count)}, {"digest", QString::number(expected.hash, 16)}});
        }
        if (mixed) {
            QJsonArray scaling;
            double previousMs = 0;
            for (const int mib : {25, 50, 100}) {
                QList<double> samples;
                Timing result;
                for (int round = 0; round < 3; ++round) {
                    result = timed(false, corpus, mib * 1024 * 1024, 65536, true,
                                   directory + "/diagnostics-" + QString::number(mib) + "MiB.log");
                    samples.append(result.milliseconds);
                }
                std::sort(samples.begin(), samples.end());
                const double median = samples[1];
                const double growth = previousMs > 0 ? median / previousMs : 0;
                passed = passed && (previousMs == 0 || growth <= 3.0);
                scaling.append(QJsonObject{{"mib", mib}, {"median_ms", median}, {"doubling_ratio", growth},
                    {"messages", double(result.messages)}, {"log_bytes", double(result.logBytes)},
                    {"maximum_feed_ms_last_round", result.maximumFeedMs}});
                progress(QString("LOG_SCALING mib=%1 median_ms=%2 growth=%3 messages=%4")
                         .arg(mib).arg(median).arg(growth).arg(result.messages));
                previousMs = median;
            }
            results.append(QJsonObject{{"logging_scaling", scaling}});
        }
    }
    return {{"baseline_commit", COMPARISON_BASELINE}, {"passed", passed}, {"no_log_ratio_limit", 1.15},
        {"logging_doubling_limit", 3.0}, {"rounds", 3}, {"corpora", corpora}, {"results", results}};
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const auto previous = qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules("protocol.comparison.info=false");
    try {
        const auto args = app.arguments();
        if (args.size() == 4 && args[1] == "--probe") return probe(args[2], args[3] == "baseline");
        const QString directory = "comparison-results";
        require(QDir().mkpath(directory), "cannot create comparison result directory");
        if (args.contains("--correctness")) {
            writeJson(directory + "/correctness.json", correctness(directory));
            progress("CORRECTNESS_RESULT passed");
        } else if (args.contains("--layout-matrix")) {
            const auto result = runLayoutMatrix();
            writeJson(directory + "/layout-matrix.json", result);
            require(result["passed"].toBool(), "layout matrix failed; inspect layout-matrix.json");
            progress(QString("LAYOUT_MATRIX_RESULT passed layouts=%1 partitions=%2 feeds=%3")
                .arg(result["layouts"].toInt()).arg(result["partition_cases"].toDouble(), 0, 'f', 0)
                .arg(result["feed_calls"].toDouble(), 0, 'f', 0));
        } else if (args.contains("--performance")) {
            const auto result = performance(directory);
            writeJson(directory + "/performance.json", result);
            require(result["passed"].toBool(), "performance acceptance threshold exceeded; inspect performance.json");
            progress("PERFORMANCE_RESULT passed");
        } else {
            throw std::runtime_error("use --correctness, --layout-matrix or --performance");
        }
        qInstallMessageHandler(previous);
        return 0;
    } catch (const std::exception& error) {
        diagnosticFile = nullptr;
        qInstallMessageHandler(previous);
        progress(QString("COMPARISON_FAILED %1").arg(QString::fromUtf8(error.what())));
        return 1;
    }
}
