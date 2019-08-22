#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractTelnet.h"

#include <QByteArray>
#include <QObject>

class MudTelnet final : public AbstractTelnet
{
    Q_OBJECT
public:
    explicit MudTelnet(QObject *parent);
    ~MudTelnet() override = default;

public slots:
    void onAnalyzeMudStream(const QByteArray &);
    void onSendToMud(const QByteArray &);
    void onConnected();
    void onRelayNaws(int, int);
    void onRelayTermType(QByteArray);

signals:
    void analyzeMudStream(const QByteArray &, bool goAhead);
    void sendToSocket(const QByteArray &);
    void relayEchoMode(bool);

private:
    void sendToMapper(const QByteArray &data, bool goAhead) override;
    void receiveEchoMode(bool toggle) override;
    void sendRawData(const QByteArray &data) override;
};
