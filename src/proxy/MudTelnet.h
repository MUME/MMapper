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
    ~MudTelnet() final = default;

public slots:
    void slot_onAnalyzeMudStream(const QByteArray &);
    void slot_onSendToMud(const QByteArray &);
    void slot_onConnected();
    void slot_onRelayNaws(int, int);
    void slot_onRelayTermType(const QByteArray &);
    void slot_onGmcpToMud(const GmcpMessage &);

signals:
    void sig_analyzeMudStream(const QByteArray &, bool goAhead);
    void sig_sendToSocket(const QByteArray &);
    void sig_relayEchoMode(bool);
    void sig_relayGmcp(const GmcpMessage &);

private:
    void virt_sendToMapper(const QByteArray &data, bool goAhead) final;
    void virt_receiveEchoMode(bool toggle) final;
    void virt_receiveGmcpMessage(const GmcpMessage &) final;
    void virt_onGmcpEnabled() final;
    void virt_sendRawData(const std::string_view &data) final;
};
