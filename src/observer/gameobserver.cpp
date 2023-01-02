#include "gameobserver.h"

GameObserver::GameObserver(QObject *parent)
    : QObject{parent}
{}

void GameObserver::slot_sendtoMud(const QByteArray &ba)
{
    emit sig_sendToMud(ba);
}

void GameObserver::slot_sendToUser(const QByteArray &ba, bool goAhead)
{
    emit sig_sendToUser(ba, goAhead);
}

void GameObserver::slot_sendGmcpToMud(const GmcpMessage &m)
{
    emit sig_sendGmcpToMud(m);
}

void GameObserver::slot_sendGmcpToUser(const GmcpMessage &m)
{
    emit sig_sendGmcpToUser(m);
}

void GameObserver::slot_log(const QString &ba, const QString &s)
{
    emit sig_log(ba, s);
}

