#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractTelnet.h"

#include <QByteArray>
#include <QObject>

class UserTelnet final : public AbstractTelnet
{
    Q_OBJECT
public:
    explicit UserTelnet(QObject *parent);
    ~UserTelnet() override = default;

public slots:
    void onSendToUser(const QByteArray &data, bool goAhead);
    void onAnalyzeUserStream(const QByteArray &);
    void onConnected();
    void onRelayEchoMode(bool);

signals:
    void analyzeUserStream(const QByteArray &, bool goAhead);
    void sendToSocket(const QByteArray &);

    void relayNaws(int, int);
    void relayTermType(QByteArray);

private:
    void sendToMapper(const QByteArray &data, bool goAhead) override;
    void receiveTerminalType(const QByteArray &) override;
    void receiveWindowSize(int, int) override;
    void sendRawData(const QByteArray &data) override;
};
