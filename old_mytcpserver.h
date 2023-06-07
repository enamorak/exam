#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>

class MyTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MyTcpServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void readyRead();
    void disconnected();

private:
    QList<QTcpSocket*> m_clients;
    QMap<QString, QList<QString>> m_rooms;
    QMap<QString, QString> m_waiting;
    int m_clientsCount = 0;

    void broadcast(const QString &message, QTcpSocket *excludeClient = nullptr);
    void sendToClient(QTcpSocket *client, const QString &message);
    void removeClient(QTcpSocket *client);
    void createRoom(const QString &name);
    void addClientToRoom(const QString &login, const QString &roomName);
    void removeClientFromRoom(const QString &login, const QString &roomName);
    QString getWaitingList();
    QString getRoomsList();
};

#endif // MYTCPSERVER_H
