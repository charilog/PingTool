#include "PingCommandBuilder.h"
#include <QOperatingSystemVersion>
#include <QtGlobal>

static bool isWindows()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

static bool isMac()
{
#ifdef Q_OS_MAC
    return true;
#else
    return false;
#endif
}

Command PingCommandBuilder::buildPing(const QString& host, const PingOptions& opt)
{
    Command c;

    // Always start the program directly (no shell) to avoid injection issues.
    c.program = "ping";

    if (isWindows())
    {
        // Windows ping:
        //  -n <count>, -t (continuous), -w <timeout_ms>, -l <size>, -4 / -6
        if (opt.ipv6) c.args << "-6";
        else c.args << "-4";

        if (opt.count <= 0) c.args << "-t";
        else c.args << "-n" << QString::number(opt.count);

        c.args << "-w" << QString::number(opt.timeoutMs);
        c.args << "-l" << QString::number(opt.payloadBytes);

        // Interval is not configurable in Windows ping; ignored.
    }
    else
    {
        // Linux/macOS ping:
        //  -c <count>, (no count => continuous), -W <timeout>, -i <interval>, -s <size>, -4/-6
        // Note: exact meaning of -W differs a bit across platforms; best effort.
        if (opt.ipv6) c.args << "-6";
        else c.args << "-4";

        if (opt.count > 0) c.args << "-c" << QString::number(opt.count);

        // timeout: prefer whole seconds for -W where required
        int timeoutSec = qMax(1, (opt.timeoutMs + 999) / 1000);
        c.args << "-W" << QString::number(timeoutSec);

        // interval (Linux allows decimal; macOS does too). Some systems require root for very small values.
        double interval = opt.intervalSec;
        if (interval < 0.2) interval = 0.2;
        c.args << "-i" << QString::number(interval, 'f', 3);

        c.args << "-s" << QString::number(qMax(0, opt.payloadBytes));
    }

    c.args << host.trimmed();
    return c;
}

Command PingCommandBuilder::buildTraceroute(const QString& host, bool ipv6)
{
    Command c;
    if (isWindows())
    {
        c.program = "tracert";
        if (ipv6) c.args << "-6";
        else c.args << "-4";
        c.args << host.trimmed();
        return c;
    }

    // Linux/macOS
    c.program = "traceroute";
    if (ipv6) c.args << "-6";
    else c.args << "-4";
    c.args << host.trimmed();
    return c;
}
