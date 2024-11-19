#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractTelnet.h"

#include <QByteArray>
#include <QObject>

class NODISCARD_QOBJECT MudTelnet final : public AbstractTelnet
{
    Q_OBJECT

private:
    /** modules for GMCP */
    GmcpModuleSet m_gmcp;
    bool m_receivedExternalDiscordHello = false;

public:
    explicit MudTelnet(QObject *parent);
    ~MudTelnet() final = default;

private:
    void virt_sendToMapper(const RawBytes &data, bool goAhead) final;
    void virt_receiveEchoMode(bool toggle) final;
    void virt_receiveGmcpMessage(const GmcpMessage &) final;
    void virt_receiveMudServerStatus(const TelnetMsspBytes &) final;
    void virt_onGmcpEnabled() final;
    void virt_sendRawData(const TelnetIacBytes &data) final;

private:
    void receiveGmcpModule(const GmcpModule &, bool);
    void resetGmcpModules();
    void parseMudServerStatus(const TelnetMsspBytes &);

private:
    void sendCoreSupports();

signals:
    void sig_analyzeMudStream(const RawBytes &, bool goAhead);
    void sig_sendToSocket(const TelnetIacBytes &);
    void sig_relayEchoMode(bool);
    void sig_relayGmcp(const GmcpMessage &);
    void sig_sendMSSPToUser(const TelnetMsspBytes &);
    void sig_sendGameTimeToClock(int year, const std::string &month, int day, int hour);

public slots:
    void slot_onAnalyzeMudStream(const TelnetIacBytes &);
    void slot_onSendRawToMud(const RawBytes &);
    void slot_onSendToMud(const QString &);
    void slot_onDisconnected();
    void slot_onRelayNaws(int, int);
    void slot_onRelayTermType(const TelnetTermTypeBytes &);
    void slot_onGmcpToMud(const GmcpMessage &);
};
