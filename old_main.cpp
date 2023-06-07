#include <QCoreApplication>
#include <QTcpSocket>
#include <QDebug>
#include "mytcpserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MyTcpServer server;
    if (!server.listen(QHostAddress::Any, 3333)) {
        qDebug() << "Failed to start server:" << server.errorString();
        return 1;
    }
    qDebug() << "Server started on port" << server.serverPort();

    return a.exec();
}
