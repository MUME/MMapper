#ifndef GAMEOBSERVER_H
#define GAMEOBSERVER_H

#include "proxy/GmcpMessage.h"
#include <QObject>

class GameObserver : public QObject
{
    Q_OBJECT
public:
    explicit GameObserver(QObject *parent = nullptr);

signals:
    void sig_sentToMudText(const QByteArray &);
    void sig_sentToUserText(const QByteArray &, bool goAhead);

    void sig_sentToMudGmcp(const GmcpMessage &);
    void sig_sentToUserGmcp(const GmcpMessage &);

    //void sig_mapChanged();
    //void sig_graphicsSettingsChanged();
    //void sig_releaseAllPaths();

    // used to log
    void sig_log(const QString &, const QString &);

public slots:
    void slot_observeSentToMudText(const QByteArray &ba);
    void slot_observeSentToUserText(const QByteArray &ba, bool goAhead);

    void slot_observeSentToMudGmcp(const GmcpMessage &m);
    void slot_observeSentToUserGmcp(const GmcpMessage &m);

    void slot_log(const QString &ba, const QString &);
};

#endif // GAMEOBSERVER_H
