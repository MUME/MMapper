#include "gameobserver.h"

#include "parser/parserutils.h"

GameObserver::GameObserver(QObject *const parent)
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

void GameObserver::slot_observeSentToMud(const QByteArray &ba)
{
    emit sig_sentToMudBytes(ba);
    QString str = QString::fromLatin1(ba);
    ParserUtils::removeAnsiMarksInPlace(str);
    emit sig_sentToMudString(str);
}

void GameObserver::slot_observeSentToUser(const QByteArray &ba, bool goAhead)
{
    emit sig_sentToUserBytes(ba, goAhead);
    QString str = QString::fromLatin1(ba);
    ParserUtils::removeAnsiMarksInPlace(str);
    emit sig_sentToUserString(str);
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
