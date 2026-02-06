#include "PingOutputParser.h"
#include <QRegularExpression>

static bool parseWindowsPackets(const QString& t, PingStats& s)
{
    // Example:
    // Packets: Sent = 4, Received = 4, Lost = 0 (0% loss),
    QRegularExpression re(R"(Packets:\s*Sent\s*=\s*(\d+),\s*Received\s*=\s*(\d+),\s*Lost\s*=\s*(\d+)\s*\((\d+)%\s*loss\))",
                          QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(t);
    if (!m.hasMatch()) return false;
    s.hasPacketStats = true;
    s.sent = m.captured(1).toInt();
    s.received = m.captured(2).toInt();
    s.lost = m.captured(3).toInt();
    s.lossPct = m.captured(4).toDouble();
    return true;
}

static bool parseWindowsRtt(const QString& t, PingStats& s)
{
    // Example:
    // Minimum = 1ms, Maximum = 2ms, Average = 1ms
    QRegularExpression re(R"(Minimum\s*=\s*(\d+)\s*ms,\s*Maximum\s*=\s*(\d+)\s*ms,\s*Average\s*=\s*(\d+)\s*ms)",
                          QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(t);
    if (!m.hasMatch()) return false;
    s.hasRtt = true;
    s.rttMinMs = m.captured(1).toDouble();
    s.rttMaxMs = m.captured(2).toDouble();
    s.rttAvgMs = m.captured(3).toDouble();
    s.rttMdevMs = -1.0;
    return true;
}

static bool parseUnixPackets(const QString& t, PingStats& s)
{
    // Linux example:
    // 4 packets transmitted, 4 received, 0% packet loss, time 3060ms
    QRegularExpression re(R"((\d+)\s+packets\s+transmitted,\s+(\d+)\s+(?:packets\s+)?received,\s+(?:\+?(\d+)\s+errors,\s+)?([0-9.]+)%\s+packet\s+loss)",
                          QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(t);
    if (!m.hasMatch()) return false;
    s.hasPacketStats = true;
    s.sent = m.captured(1).toInt();
    s.received = m.captured(2).toInt();
    s.lossPct = m.captured(4).toDouble();
    if (s.sent >= 0 && s.received >= 0) {
        s.lost = s.sent - s.received;
    }
    return true;
}

static bool parseUnixRtt(const QString& t, PingStats& s)
{
    // Linux example:
    // rtt min/avg/max/mdev = 0.051/0.060/0.069/0.007 ms
    // macOS example:
    // round-trip min/avg/max/stddev = 10.123/11.456/12.789/0.321 ms
    QRegularExpression re(R"((?:rtt|round-trip)\s+min/avg/max/(?:mdev|stddev)\s*=\s*([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+)\s*ms)",
                          QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(t);
    if (!m.hasMatch()) return false;
    s.hasRtt = true;
    s.rttMinMs = m.captured(1).toDouble();
    s.rttAvgMs = m.captured(2).toDouble();
    s.rttMaxMs = m.captured(3).toDouble();
    s.rttMdevMs = m.captured(4).toDouble();
    return true;
}

PingStats PingOutputParser::parse(const QString& fullText)
{
    PingStats s;

    // Prefer scanning line by line for summary lines, but also allow a match across whole text.
    const auto lines = fullText.split(QRegularExpression(R"(\r?\n)"));
    for (const auto& line : lines)
    {
        parseWindowsPackets(line, s);
        parseWindowsRtt(line, s);
        parseUnixPackets(line, s);
        parseUnixRtt(line, s);
    }

    // Fallback: try whole text if line-based missed.
    if (!s.hasPacketStats) {
        parseWindowsPackets(fullText, s);
        parseUnixPackets(fullText, s);
    }
    if (!s.hasRtt) {
        parseWindowsRtt(fullText, s);
        parseUnixRtt(fullText, s);
    }
    return s;
}

int PingOutputParser::countRepliesInChunk(const QString& chunk)
{
    // Heuristic: count lines that look like replies.
    // Windows: "Reply from ..."
    // Unix: "64 bytes from ..." or "bytes from ..."
    int count = 0;
    const auto lines = chunk.split(QRegularExpression(R"(\r?\n)"));
    QRegularExpression reWin(R"(^\s*Reply\s+from\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reUnix(R"(^\s*\d+\s+bytes\s+from\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reUnix2(R"(^\s*bytes\s+from\s+)", QRegularExpression::CaseInsensitiveOption);

    for (const auto& line : lines)
    {
        if (reWin.match(line).hasMatch() || reUnix.match(line).hasMatch() || reUnix2.match(line).hasMatch())
            ++count;
    }
    return count;
}
