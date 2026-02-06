#include <QApplication>
#include "PingToolWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    PingToolWindow w;
    w.show();
    return app.exec();
}
