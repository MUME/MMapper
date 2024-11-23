// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "gameobserver.h"

#include "../global/parserutils.h"

void GameObserver::slot_observeConnected()
{
    sig_connected();
}

void GameObserver::slot_observeDisconnected()
{
    sig_disconnected();
}

void GameObserver::slot_observeSentToMud(const QByteArray &ba)
{
    sig_sentToMudBytes(ba);
    QString str = QString::fromLatin1(ba);
    ParserUtils::removeAnsiMarksInPlace(str);
    sig_sentToMudString(str);
}

void GameObserver::slot_observeSentToUser(const QByteArray &ba, const bool goAhead)
{
    sig_sentToUserBytes(ba, goAhead);
    QString str = QString::fromLatin1(ba);
    ParserUtils::removeAnsiMarksInPlace(str);
    sig_sentToUserString(str);
}

void GameObserver::slot_observeSentToMudGmcp(const GmcpMessage &m)
{
    sig_sentToMudGmcp(m);
}

void GameObserver::slot_observeSentToUserGmcp(const GmcpMessage &m)
{
    sig_sentToUserGmcp(m);
}

void GameObserver::slot_observeToggledEchoMode(bool echo)
{
    sig_toggledEchoMode(echo);
}
