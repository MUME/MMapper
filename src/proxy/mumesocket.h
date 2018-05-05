#ifndef MUMESOCKET_H
#define MUMESOCKET_H

#include <QObject>
#include <QAbstractSocket>

class QTcpSocket;
class QWebSocket;

class MumeSocket : public QObject
{
    Q_OBJECT
public:
    explicit MumeSocket(QObject *parent = nullptr);

    virtual void disconnectFromHost() = 0;
    virtual void connectToHost() = 0;
    virtual void sendToMud(const QByteArray &ba) = 0;

signals:
    void connected();
    void disconnected();
    void socketError(QAbstractSocket::SocketError);
    void processMudStream(const QByteArray& buffer);

};

class MumeMudSocket : public MumeSocket
{
    Q_OBJECT
public:
    MumeMudSocket(QObject *parent);
    ~MumeMudSocket();

    void disconnectFromHost();
    void connectToHost();
    QAbstractSocket::SocketError error();
    void sendToMud(const QByteArray &ba);

protected slots:
    void onConnect();
    void onDisconnect();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError e);

private:
    char m_buffer[ 8192 ];

    QTcpSocket* m_socket;

};

#endif // MUMESOCKET_H
