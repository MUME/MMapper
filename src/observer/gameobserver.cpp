#include "gameobserver.h"

GameObserver::GameObserver(QObject *parent)
    : QObject{parent}
{}

void GameObserver::slot_observeConnected()
{
    emit sig_connected();
}

void GameObserver::slot_observeDisconnected()
{
    emit sig_disconnected();
}

void GameObserver::slot_observeSentToMudText(const QByteArray &ba)
{
    emit sig_sentToMudText(ba);
}

void GameObserver::slot_observeSentToUserText(const QByteArray &ba, bool goAhead)
{
    emit sig_sentToUserText(ba, goAhead);
}

void GameObserver::slot_observeSentToMudGmcp(const GmcpMessage &m)
{
    emit sig_sentToMudGmcp(m);
}

void GameObserver::slot_observeSentToUserGmcp(const GmcpMessage &m)
{
    emit sig_sentToUserGmcp(m);
}

void GameObserver::slot_observeToggledEchoMode(bool echo)
{
    emit sig_toggledEchoMode(echo);
}

void GameObserver::slot_log(const QString &ba, const QString &s)
{
    emit sig_log(ba, s);
}
