#pragma once
#include <QString>

struct PingStats
{
    bool hasPacketStats = false;
    int sent = -1;
    int received = -1;
    int lost = -1;
    double lossPct = -1.0;

    bool hasRtt = false;
    double rttMinMs = -1.0;
    double rttAvgMs = -1.0;
    double rttMaxMs = -1.0;
    double rttMdevMs = -1.0; // Linux: mdev; Windows: not provided
};

class PingOutputParser
{
public:
    // Parse the *final* output text from system ping.
    static PingStats parse(const QString& fullText);

    // Lightweight heuristic to count "reply" lines for progress.
    static int countRepliesInChunk(const QString& chunk);
};
