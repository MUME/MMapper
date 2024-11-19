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
    Signal2<> sig_connected;
    Signal2<> sig_disconnected;
    // Signal2<QByteArray> sig_sentToMudBytes;
    // Signal2<QByteArray, bool /* go ahead */> sig_sentToUserBytes;

    // Helper versions of the above, which remove ANSI in place and create QString
    Signal2<QString> sig_sentToMudString;
    Signal2<QString> sig_sentToUserString;
    Signal2<GmcpMessage> sig_sentToMudGmcp;
    Signal2<GmcpMessage> sig_sentToUserGmcp;
    Signal2<bool> sig_toggledEchoMode;

public:
    void slot_observeConnected();
    void slot_observeDisconnected();

    void slot_observeSentToMud(const QString &ba);
    void slot_observeSentToUser(const QString &ba, bool goAhead);

    void slot_observeSentToMudGmcp(const GmcpMessage &m);
    void slot_observeSentToUserGmcp(const GmcpMessage &m);

    void slot_observeToggledEchoMode(bool echo);
};
