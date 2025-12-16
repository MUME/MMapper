#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/ChangeMonitor.h"
#include "../global/Signal2.h"
#include "../proxy/GmcpMessage.h"

#include <QObject>

class NODISCARD_QOBJECT GameObserver final
{
public:
    Signal2<> sig2_connected;

    Signal2<QString> sig2_sentToMudString;  // removes ANSI
    Signal2<QString> sig2_sentToUserString; // removes ANSI

    Signal2<GmcpMessage> sig2_sentToUserGmcp;
    Signal2<bool> sig2_toggledEchoMode;
    Signal2<QString> sig2_rawGameText; // raw text from MUD parser (for fallback parsing)

public:
    void observeConnected();
    void observeSentToMud(const QString &ba);
    void observeSentToUser(const QString &ba);
    void observeSentToUserGmcp(const GmcpMessage &m);
    void observeToggledEchoMode(bool echo);
    void observeRawGameText(const QString &text);
};
