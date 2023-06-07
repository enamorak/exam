#include "mytcpserver.h"
#include <QDebug>
#include <QDataStream>
#include <QMap>

MyTcpServer::MyTcpServer(QObject *parent) : QTcpServer(parent)
{
}

QString MyTcpServer::getKeyByValue(QTcpSocket *value)
{
    for (auto it = m_waiting.begin(); it != m_waiting.end(); ++it) {
        if (it.value() == value) {
            return it.key();
        }
    }
    return QString();
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *client = new QTcpSocket(this);
    client->setSocketDescriptor(socketDescriptor);
    connect(client, &QTcpSocket::readyRead, this, &MyTcpServer::readyRead);
    connect(client, &QTcpSocket::disconnected, this, &MyTcpServer::disconnected);
    m_clients.append(client);
    qDebug() << "New client connected:" << client->peerAddress().toString() << ":" << client->peerPort();
}

void MyTcpServer::readyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client)
        return;

    QDataStream in(client);
    in.setVersion(QDataStream::Qt_5_15);

    QString message;
    in >> message;

    QStringList parts = message.split("&");

    if (parts[0] == "start") {
        QString login = parts[1];
        QString roomName = parts[2];
        addClientToRoom(login, roomName);
    }
    else if (parts[0] == "break") {
        QString login = m_waiting.key(client);
        if (!login.isEmpty()) {
            m_waiting.remove(login);
            sendToClient(client, "You have been removed from the waiting list.");
        }
    }
    else if (parts[0] == "stats") {
        QString waitingList = getWaitingList();
        if (!waitingList.isEmpty()) {
            sendToClient(client, "Waiting list:\n" + waitingList);
        }
        else {
            sendToClient(client, "No clients are waiting.");
        }
    }
    else if (parts[0] == "rooms") {
        QString roomsList = getRoomsList();
        if (!roomsList.isEmpty()) {
            sendToClient(client, "Rooms:\n" + roomsList);
        }
        else {
            sendToClient(client, "No rooms available.");
        }
    }
    else if (parts[0] == "newroom") {
        QString name = parts[1];
        createRoom(name);
        sendToClient(client, "Room " + name + " has been created.");
    }
}

void MyTcpServer::disconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client)
        return;

    removeClient(client);
}

void MyTcpServer::broadcast(const QString &message, QTcpSocket *excludeClient)
{
    for (QTcpSocket *client : m_clients) {
        if (client != excludeClient) {
            sendToClient(client, message);
        }
    }
}

void MyTcpServer::sendToClient(QTcpSocket *client, const QString &message)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << message;
    client->write(block);
}

void MyTcpServer::removeClient(QTcpSocket *client)
{
    QString login = m_waiting.key(client);
    if (!login.isEmpty()) {
        m_waiting.remove(login);
        broadcast(login + " has been removed from the waiting list.");
    }
    else {
        for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
            if (it.value().contains(login)) {
                removeClientFromRoom(login, it.key());
                break;
            }
        }
    }

    m_clients.removeOne(client);
    client->deleteLater();
    qDebug() << "Client disconnected:" << client->peerAddress().toString() << ":" << client->peerPort();
}

void MyTcpServer::createRoom(const QString &name)
{
    if (!m_rooms.contains(name)) {
        m_rooms.insert(name, QList<QString>());
    }
}

void MyTcpServer::addClientToRoom(const QString &login, const QString &roomName)
{
    if (m_waiting.contains(login)) {
        sendToClient(m_waiting[login], "You have been removed from the waiting list.");
        m_waiting.remove(login);
    }

    if (m_rooms.contains(roomName)) {
        if (!m_rooms[roomName].contains(login)) {
            m_rooms[roomName].append(login);
            if (m_rooms[roomName].size() == 7) {
                QString message = "Game starting with players:";
                for (QString player : m_rooms[roomName]) {
                    message += " " + player;
                }
                broadcast(message);
                for (QString player : m_rooms[roomName]) {
                    QTcpSocket *client = m_waiting[player];
                    removeClient(client);
                }
                m_rooms.remove(roomName);
            }
            else {
                broadcast(login + " joined room " + roomName);
            }
        }
        else {
            sendToClient(m_waiting[login], "You are already in room " + roomName);
        }
    }
    else {
        sendToClient(m_waiting[login], "Room " + roomName + " does not exist.");
    }
}

void MyTcpServer::removeClientFromRoom(const QString &login, const QString &roomName)
{
    if (m_rooms.contains(roomName)) {
        if (m_rooms[roomName].contains(login)) {
            m_rooms[roomName].removeOne(login);
            broadcast(login + " left room " + roomName);
        }
    }
}

QString MyTcpServer::getWaitingList()
{
    QString waitingList;
    for (QString login : m_waiting.keys()) {
        waitingList += login + "\n";
    }
    return waitingList;
}

QString MyTcpServer::getRoomsList()
{
    QString roomsList;
    for (QString roomName : m_rooms.keys()) {
        roomsList += roomName + " (" + QString::number(m_rooms[roomName].size()) + "/7)\n";
    }
    return roomsList;
}
