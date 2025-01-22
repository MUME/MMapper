// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "gameobserver.h"

#include "../global/parserutils.h"

void GameObserver::observeConnected()
{
    sig2_connected.invoke();
}

void GameObserver::observeSentToMud(const QString &input)
{
    auto str = input;
    ParserUtils::removeAnsiMarksInPlace(str);
    sig2_sentToMudString.invoke(str);
}

void GameObserver::observeSentToUser(const QString &input)
{
    auto str = input;
    ParserUtils::removeAnsiMarksInPlace(str);
    sig2_sentToUserString.invoke(str);
}

void GameObserver::observeSentToUserGmcp(const GmcpMessage &m)
{
    sig2_sentToUserGmcp.invoke(m);
}

void GameObserver::observeToggledEchoMode(const bool echo)
{
    sig2_toggledEchoMode.invoke(echo);
}
