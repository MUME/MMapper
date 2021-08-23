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
    ~UserTelnet() final = default;

public slots:
    void slot_onSendToUser(const QByteArray &data, bool goAhead);
    void slot_onAnalyzeUserStream(const QByteArray &);
    void slot_onConnected();
    void slot_onRelayEchoMode(bool);
    void slot_onGmcpToUser(const GmcpMessage &);

signals:
    void sig_analyzeUserStream(const QByteArray &, bool goAhead);
    void sig_sendToSocket(const QByteArray &);
    void sig_relayGmcp(const GmcpMessage &);
    void sig_relayNaws(int, int);
    void sig_relayTermType(QByteArray);

private:
    void virt_sendToMapper(const QByteArray &data, bool goAhead) final;
    void virt_receiveGmcpMessage(const GmcpMessage &) final;
    void virt_receiveTerminalType(const QByteArray &) final;
    void virt_receiveWindowSize(int, int) final;
    void virt_sendRawData(const std::string_view &data) final;
};
