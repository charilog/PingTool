#pragma once
#include <QString>
#include <QStringList>

struct PingOptions
{
    int count = 4;              // 0 => continuous
    int timeoutMs = 1000;       // per-packet timeout (best effort across OS)
    double intervalSec = 1.0;   // best effort; may be ignored on Windows
    int payloadBytes = 32;      // ICMP payload size (best effort)
    bool ipv6 = false;
};

struct Command
{
    QString program;
    QStringList args;
};

class PingCommandBuilder
{
public:
    static Command buildPing(const QString& host, const PingOptions& opt);
    static Command buildTraceroute(const QString& host, bool ipv6);
};
