#ifndef GAMEOBSERVER_H
#define GAMEOBSERVER_H

#include <QObject>

class GameObserver : public QObject
{
    Q_OBJECT
public:
    explicit GameObserver(QObject *parent = nullptr);

signals:
    void sig_sendToMud(const QByteArray &);
    void sig_sendToUser(const QByteArray &, bool goAhead);
    //void sig_mapChanged();
    //void sig_graphicsSettingsChanged();
    //void sig_releaseAllPaths();

    // used to log
    void sig_log(const QString &, const QString &);

public slots:
    void slot_sendtoMud(const QByteArray &ba);
    void slot_sendToUser(const QByteArray &ba, bool goAhead);
    void slot_log(const QString &ba, const QString &);
};

#endif // GAMEOBSERVER_H
