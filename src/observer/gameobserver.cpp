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

void GameObserver::slot_observeSentToMud(const QString &input)
{
    // sig_sentToMudBytes(input.toUtf8()); // FIXME: This does not make any sense anymore.
    auto str = input;
    ParserUtils::removeAnsiMarksInPlace(str);
    sig_sentToMudString(str);
}

void GameObserver::slot_observeSentToUser(const QString &input, const bool goAhead)
{
    // sig_sentToUserBytes(input.toUtf8(), goAhead); // FIXME: This does not make any sense anymore.
    auto str = input;
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

void GameObserver::slot_observeToggledEchoMode(const bool echo)
{
    sig_toggledEchoMode(echo);
}
