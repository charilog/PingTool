#pragma once
#include <QMainWindow>
#include <QProcess>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTextEdit;
class QProgressBar;
class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QGroupBox;
class QTabWidget;
QT_END_NAMESPACE

class PingToolWindow final : public QMainWindow
{
    Q_OBJECT

public:
    PingToolWindow();
    ~PingToolWindow() override = default;

private slots:
    void onPingClicked();
    void onStopClicked();
    void onTracerouteClicked();
    void onDnsClicked();
    void onTcpTestClicked();
    void onClearClicked();
    void onSaveClicked();
    void onCopyClicked();

    void onProcReadyRead();
    void onProcFinished(int exitCode, QProcess::ExitStatus status);
    void onProcError(QProcess::ProcessError err);

private:
    void setRunning(bool running);
    void appendOutput(const QString& text);
    void startCommand(const QString& program, const QStringList& args, const QString& headerLine);
    QStringList splitHosts(const QString& input) const;
    void startNextHostIfAny();
    void updateStatsUI(const QString& fullText);
    void updateProgress(bool finished = false);

    // UI
    QLineEdit* hostEdit_ = nullptr;
    QPushButton* pingBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* traceBtn_ = nullptr;
    QPushButton* dnsBtn_ = nullptr;
    QPushButton* tcpBtn_ = nullptr;
    QPushButton* clearBtn_ = nullptr;
    QPushButton* saveBtn_ = nullptr;
    QPushButton* copyBtn_ = nullptr;

    QSpinBox* countSpin_ = nullptr;
    QSpinBox* timeoutSpin_ = nullptr;
    QDoubleSpinBox* intervalSpin_ = nullptr;
    QSpinBox* payloadSpin_ = nullptr;
    QCheckBox* ipv6Chk_ = nullptr;
    QCheckBox* continuousChk_ = nullptr;

    QSpinBox* tcpPortSpin_ = nullptr;

    QTextEdit* output_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* pktLabel_ = nullptr;
    QLabel* rttLabel_ = nullptr;

    // Process execution
    QProcess proc_;
    QString currentProgram_;
    QStringList currentArgs_;
    QString fullText_;
    QStringList hostQueue_;
    int totalExpectedReplies_ = 0;
    int repliesSoFar_ = 0;

    // TCP test
    QElapsedTimer tcpTimer_;
};
