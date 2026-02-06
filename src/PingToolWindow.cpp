#include "PingToolWindow.h"
#include "PingCommandBuilder.h"
#include "PingOutputParser.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHostInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTcpSocket>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QRegularExpression>
#include <QTextCursor>

static QString nowStamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

PingToolWindow::PingToolWindow()
{
    setWindowTitle("Ping tool by charilog v1.0 (C++/Qt)");

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    // Top row: host + buttons
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Host(s):", this));

    hostEdit_ = new QLineEdit(this);
    hostEdit_->setPlaceholderText("e.g. www.example.com or 8.8.8.8 (comma/space separated)");
    hostEdit_->setText("www.ce.teiep.gr");
    topRow->addWidget(hostEdit_, 1);

    pingBtn_ = new QPushButton("Ping", this);
    stopBtn_ = new QPushButton("Stop", this);
    traceBtn_ = new QPushButton("Traceroute", this);
    dnsBtn_ = new QPushButton("DNS", this);
    tcpBtn_ = new QPushButton("TCP Test", this);

    topRow->addWidget(pingBtn_);
    topRow->addWidget(stopBtn_);
    topRow->addWidget(traceBtn_);
    topRow->addWidget(dnsBtn_);
    topRow->addWidget(tcpBtn_);

    root->addLayout(topRow);

    // Options row
    auto* optBox = new QGroupBox("Options", this);
    auto* opt = new QHBoxLayout(optBox);

    countSpin_ = new QSpinBox(this);
    countSpin_->setRange(1, 1000);
    countSpin_->setValue(4);

    continuousChk_ = new QCheckBox("Continuous", this);

    timeoutSpin_ = new QSpinBox(this);
    timeoutSpin_->setRange(100, 60000);
    timeoutSpin_->setValue(1000);

    intervalSpin_ = new QDoubleSpinBox(this);
    intervalSpin_->setRange(0.2, 10.0);
    intervalSpin_->setDecimals(2);
    intervalSpin_->setSingleStep(0.1);
    intervalSpin_->setValue(1.0);

    payloadSpin_ = new QSpinBox(this);
    payloadSpin_->setRange(0, 65000);
    payloadSpin_->setValue(32);

    ipv6Chk_ = new QCheckBox("IPv6", this);

    tcpPortSpin_ = new QSpinBox(this);
    tcpPortSpin_->setRange(1, 65535);
    tcpPortSpin_->setValue(443);

    opt->addWidget(new QLabel("Count:", this));
    opt->addWidget(countSpin_);
    opt->addWidget(continuousChk_);
    opt->addSpacing(10);
    opt->addWidget(new QLabel("Timeout (ms):", this));
    opt->addWidget(timeoutSpin_);
    opt->addWidget(new QLabel("Interval (s):", this));
    opt->addWidget(intervalSpin_);
    opt->addWidget(new QLabel("Payload (B):", this));
    opt->addWidget(payloadSpin_);
    opt->addWidget(ipv6Chk_);
    opt->addSpacing(10);
    opt->addWidget(new QLabel("TCP Port:", this));
    opt->addWidget(tcpPortSpin_);
    opt->addStretch(1);

    root->addWidget(optBox);

    // Output
    output_ = new QTextEdit(this);
    output_->setReadOnly(true);
    output_->setMinimumHeight(260);
    root->addWidget(output_, 1);

    // Bottom row: progress + actions + stats
    auto* bottom = new QHBoxLayout();

    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setValue(0);
    progress_->setTextVisible(true);
    progress_->setVisible(false);

    clearBtn_ = new QPushButton("Clear", this);
    saveBtn_ = new QPushButton("Save", this);
    copyBtn_ = new QPushButton("Copy", this);

    statusLabel_ = new QLabel("Idle", this);
    pktLabel_ = new QLabel("Packets: -", this);
    rttLabel_ = new QLabel("RTT: -", this);

    bottom->addWidget(progress_, 0);
    bottom->addWidget(clearBtn_);
    bottom->addWidget(saveBtn_);
    bottom->addWidget(copyBtn_);
    bottom->addSpacing(15);
    bottom->addWidget(statusLabel_, 1);
    bottom->addWidget(pktLabel_, 0);
    bottom->addWidget(rttLabel_, 0);

    root->addLayout(bottom);

    setCentralWidget(central);
    resize(980, 520);

    stopBtn_->setEnabled(false);

    connect(pingBtn_, &QPushButton::clicked, this, &PingToolWindow::onPingClicked);
    connect(stopBtn_, &QPushButton::clicked, this, &PingToolWindow::onStopClicked);
    connect(traceBtn_, &QPushButton::clicked, this, &PingToolWindow::onTracerouteClicked);
    connect(dnsBtn_, &QPushButton::clicked, this, &PingToolWindow::onDnsClicked);
    connect(tcpBtn_, &QPushButton::clicked, this, &PingToolWindow::onTcpTestClicked);
    connect(clearBtn_, &QPushButton::clicked, this, &PingToolWindow::onClearClicked);
    connect(saveBtn_, &QPushButton::clicked, this, &PingToolWindow::onSaveClicked);
    connect(copyBtn_, &QPushButton::clicked, this, &PingToolWindow::onCopyClicked);

    proc_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&proc_, &QProcess::readyRead, this, &PingToolWindow::onProcReadyRead);
    connect(&proc_, &QProcess::finished, this, &PingToolWindow::onProcFinished);
    connect(&proc_, &QProcess::errorOccurred, this, &PingToolWindow::onProcError);
}

QStringList PingToolWindow::splitHosts(const QString& input) const
{
    QString s = input;
    s.replace("\n", " ");
    s.replace("\r", " ");
    s.replace(",", " ");
    s.replace(";", " ");
    const auto parts = s.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);

    QStringList out;
    for (auto p : parts)
    {
        p = p.trimmed();
        if (!p.isEmpty()) out << p;
    }
    return out;
}

void PingToolWindow::setRunning(bool running)
{
    pingBtn_->setEnabled(!running);
    traceBtn_->setEnabled(!running);
    dnsBtn_->setEnabled(!running);
    tcpBtn_->setEnabled(!running);
    stopBtn_->setEnabled(running);
    progress_->setVisible(running);
    if (!running) progress_->setValue(0);
}

void PingToolWindow::appendOutput(const QString& text)
{
    output_->moveCursor(QTextCursor::End);
    output_->insertPlainText(text);
    output_->moveCursor(QTextCursor::End);
}

void PingToolWindow::startCommand(const QString& program, const QStringList& args, const QString& headerLine)
{
    fullText_.clear();
    repliesSoFar_ = 0;

    appendOutput("\n");
    appendOutput("[" + nowStamp() + "] " + headerLine + "\n");
    appendOutput("Command: " + program + " " + args.join(' ') + "\n\n");

    currentProgram_ = program;
    currentArgs_ = args;

    proc_.start(program, args);
    if (!proc_.waitForStarted(1500))
    {
        setRunning(false);
        statusLabel_->setText("Failed to start process");
        appendOutput("ERROR: Could not start command.\n");
        return;
    }

    setRunning(true);
    statusLabel_->setText("Running...");
    pktLabel_->setText("Packets: -");
    rttLabel_->setText("RTT: -");

    updateProgress(false);
}

void PingToolWindow::onPingClicked()
{
    if (proc_.state() != QProcess::NotRunning)
        return;

    hostQueue_ = splitHosts(hostEdit_->text());
    if (hostQueue_.isEmpty())
    {
        QMessageBox::warning(this, "PingTool", "Please enter at least one host.");
        return;
    }

    startNextHostIfAny();
}

void PingToolWindow::startNextHostIfAny()
{
    if (hostQueue_.isEmpty())
    {
        setRunning(false);
        statusLabel_->setText("Done");
        return;
    }

    const QString host = hostQueue_.takeFirst();

    PingOptions opt;
    opt.ipv6 = ipv6Chk_->isChecked();
    opt.payloadBytes = payloadSpin_->value();
    opt.timeoutMs = timeoutSpin_->value();
    opt.intervalSec = intervalSpin_->value();
    opt.count = continuousChk_->isChecked() ? 0 : countSpin_->value();

    totalExpectedReplies_ = (opt.count <= 0) ? 0 : opt.count;

    const Command cmd = PingCommandBuilder::buildPing(host, opt);
    startCommand(cmd.program, cmd.args, "PING " + host);
}

void PingToolWindow::onStopClicked()
{
    if (proc_.state() == QProcess::NotRunning)
    {
        setRunning(false);
        return;
    }

    appendOutput("\n[" + nowStamp() + "] STOP requested\n");
    proc_.kill();
    proc_.waitForFinished(1500);

    // Clear any remaining host queue.
    hostQueue_.clear();

    setRunning(false);
    statusLabel_->setText("Stopped");
}

void PingToolWindow::onTracerouteClicked()
{
    if (proc_.state() != QProcess::NotRunning)
        return;

    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty())
    {
        QMessageBox::warning(this, "PingTool", "Please enter a host.");
        return;
    }

    hostQueue_.clear();
    const Command cmd = PingCommandBuilder::buildTraceroute(host, ipv6Chk_->isChecked());
    totalExpectedReplies_ = 0;
    startCommand(cmd.program, cmd.args, "TRACEROUTE " + host);
}

void PingToolWindow::onDnsClicked()
{
    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty())
    {
        QMessageBox::warning(this, "PingTool", "Please enter a host.");
        return;
    }

    appendOutput("\n[" + nowStamp() + "] DNS lookup: " + host + "\n");

    QHostInfo::lookupHost(host, this, [this, host](const QHostInfo& info)
    {
        if (info.error() != QHostInfo::NoError)
        {
            appendOutput("DNS error: " + info.errorString() + "\n");
            return;
        }

        QStringList ips;
        for (const auto& addr : info.addresses())
            ips << addr.toString();

        appendOutput("Addresses: " + ips.join(", ") + "\n");

        // Reverse lookup for first address
        if (!info.addresses().isEmpty())
        {
            const auto first = info.addresses().first();
            QHostInfo::lookupHost(first.toString(), this, [this, first](const QHostInfo& rev)
            {
                if (rev.error() == QHostInfo::NoError)
                    appendOutput("Reverse (" + first.toString() + "): " + rev.hostName() + "\n");
                else
                    appendOutput("Reverse (" + first.toString() + "): " + rev.errorString() + "\n");
            });
        }
    });
}

void PingToolWindow::onTcpTestClicked()
{
    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty())
    {
        QMessageBox::warning(this, "PingTool", "Please enter a host.");
        return;
    }

    const int port = tcpPortSpin_->value();

    appendOutput("\n[" + nowStamp() + "] TCP test: " + host + ":" + QString::number(port) + "\n");

    auto* sock = new QTcpSocket(this);
    tcpTimer_.restart();
    sock->connectToHost(host, static_cast<quint16>(port));

    connect(sock, &QTcpSocket::connected, this, [this, sock]()
    {
        const qint64 ms = tcpTimer_.elapsed();
        appendOutput("TCP connect: OK (" + QString::number(ms) + " ms)\n");
        sock->disconnectFromHost();
    });

    connect(sock, &QTcpSocket::errorOccurred, this, [this, sock](QAbstractSocket::SocketError)
    {
        const qint64 ms = tcpTimer_.elapsed();
        appendOutput("TCP connect: FAIL (" + QString::number(ms) + " ms) - " + sock->errorString() + "\n");
        sock->deleteLater();
    });

    connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
}

void PingToolWindow::onClearClicked()
{
    output_->clear();
    pktLabel_->setText("Packets: -");
    rttLabel_->setText("RTT: -");
    statusLabel_->setText("Idle");
}

void PingToolWindow::onSaveClicked()
{
    const QString fn = QFileDialog::getSaveFileName(this, "Save log", "ping_log.txt", "Text files (*.txt);;All files (*)");
    if (fn.isEmpty()) return;

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        QMessageBox::warning(this, "PingTool", "Cannot write file.");
        return;
    }
    f.write(output_->toPlainText().toUtf8());
    f.close();
}

void PingToolWindow::onCopyClicked()
{
    QApplication::clipboard()->setText(output_->toPlainText());
}

void PingToolWindow::onProcReadyRead()
{
    const QString chunk = QString::fromLocal8Bit(proc_.readAll());
    fullText_ += chunk;

    if (totalExpectedReplies_ > 0)
        repliesSoFar_ += PingOutputParser::countRepliesInChunk(chunk);

    appendOutput(chunk);
    updateProgress(false);
}

void PingToolWindow::updateProgress(bool finished)
{
    if (totalExpectedReplies_ <= 0)
    {
        int v = progress_->value();
        v = (v + 7) % 101;
        progress_->setValue(v);
        progress_->setFormat("running...");
        return;
    }

    const int done = qMin(repliesSoFar_, totalExpectedReplies_);
    const int pct = (totalExpectedReplies_ == 0) ? 0 : (done * 100) / totalExpectedReplies_;
    progress_->setValue(finished ? 100 : pct);
    progress_->setFormat(QString("%1/%2").arg(done).arg(totalExpectedReplies_));
}

void PingToolWindow::updateStatsUI(const QString& fullText)
{
    const PingStats st = PingOutputParser::parse(fullText);

    if (st.hasPacketStats)
    {
        pktLabel_->setText(QString("Packets: sent %1, recv %2, loss %3%")
            .arg(st.sent).arg(st.received).arg(st.lossPct, 0, 'f', 1));
    }
    else
    {
        pktLabel_->setText("Packets: -");
    }

    if (st.hasRtt)
    {
        QString s = QString("RTT (ms): min %1, avg %2, max %3")
            .arg(st.rttMinMs, 0, 'f', 3)
            .arg(st.rttAvgMs, 0, 'f', 3)
            .arg(st.rttMaxMs, 0, 'f', 3);

        if (st.rttMdevMs >= 0.0)
            s += QString(", dev %1").arg(st.rttMdevMs, 0, 'f', 3);

        rttLabel_->setText(s);
    }
    else
    {
        rttLabel_->setText("RTT: -");
    }
}

void PingToolWindow::onProcFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);

    updateStatsUI(fullText_);
    updateProgress(true);

    if (!hostQueue_.isEmpty())
    {
        setRunning(false);
        startNextHostIfAny();
        return;
    }

    setRunning(false);
    statusLabel_->setText("Done");
}

void PingToolWindow::onProcError(QProcess::ProcessError err)
{
    Q_UNUSED(err);
    setRunning(false);
    statusLabel_->setText("Process error");
    appendOutput("\nERROR: " + proc_.errorString() + "\n");
}
