#pragma once

#include <QObject>

#include "proxy/GmcpMessage.h"

class GameObserver : public QObject
{
    Q_OBJECT
public:
    explicit GameObserver(QObject *parent = nullptr);

signals:
    void sig_connected();
    void sig_disconnected();

    void sig_sentToMudBytes(const QByteArray &);
    void sig_sentToUserBytes(const QByteArray &, bool goAhead);

    // Helper versions of the above, which remove ANSI in place and create QString
    void sig_sentToMudString(const QString &);
    void sig_sentToUserString(const QString &);

    void sig_sentToMudGmcp(const GmcpMessage &);
    void sig_sentToUserGmcp(const GmcpMessage &);

    void sig_toggledEchoMode(bool echo);

public slots:
    void slot_observeConnected();
    void slot_observeDisconnected();

    void slot_observeSentToMud(const QByteArray &ba);
    void slot_observeSentToUser(const QByteArray &ba, bool goAhead);

    void slot_observeSentToMudGmcp(const GmcpMessage &m);
    void slot_observeSentToUserGmcp(const GmcpMessage &m);

    void slot_observeToggledEchoMode(bool echo);
};
