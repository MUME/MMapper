#include "mumesocket.h"
#include "configuration/configuration.h"

#include <QTcpSocket>

MumeSocket::MumeSocket(QObject *parent) : QObject(parent)
{

}

MumeMudSocket::MumeMudSocket(QObject *parent) : MumeSocket(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(m_socket, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(onError(QAbstractSocket::SocketError)));
}

MumeMudSocket::~MumeMudSocket()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
    delete m_socket;
    m_socket = NULL;
}

void MumeMudSocket::connectToHost()
{
    m_socket->connectToHost(Config().m_remoteServerName, Config().m_remotePort, QIODevice::ReadWrite);
}

void MumeMudSocket::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

void MumeMudSocket::onConnect()
{
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, true);
    emit connected();
}

void MumeMudSocket::onDisconnect()
{
    emit disconnected();
}

void MumeMudSocket::onError(QAbstractSocket::SocketError e)
{
    emit socketError(e);
}

void MumeMudSocket::onReadyRead()
{
    int read;
    while (m_socket->bytesAvailable()) {
        read = m_socket->read(m_buffer, 8191);
        if (read != -1) {
            m_buffer[read] = 0;
            QByteArray ba = QByteArray::fromRawData(m_buffer, read);
            emit processMudStream(ba);
        }
    }
}

void MumeMudSocket::sendToMud(const QByteArray &ba)
{
    m_socket->write(ba.data(), ba.size());
    m_socket->flush();
}


