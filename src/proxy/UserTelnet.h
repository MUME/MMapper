#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractTelnet.h"

#include <QByteArray>
#include <QObject>

class NODISCARD_QOBJECT UserTelnet final : public AbstractTelnet
{
    Q_OBJECT

private:
    /** modules for GMCP */
    struct NODISCARD GmcpData final
    {
        /** MMapper relevant modules and their version */
        GmcpModuleVersionList supported;
        /** All GMCP modules */
        GmcpModuleSet modules;
    } m_gmcp{};

public:
    explicit UserTelnet(QObject *parent);
    ~UserTelnet() final = default;

private:
    NODISCARD bool virt_isGmcpModuleEnabled(const GmcpModuleTypeEnum &name) final;
    void virt_sendToMapper(const RawBytes &data, bool goAhead) final;
    void virt_receiveGmcpMessage(const GmcpMessage &) final;
    void virt_receiveTerminalType(const TelnetTermTypeBytes &) final;
    void virt_receiveWindowSize(int, int) final;
    void virt_sendRawData(const TelnetIacBytes &data) final;

private:
    void receiveGmcpModule(const GmcpModule &, bool);
    void resetGmcpModules();

signals:
    void sig_analyzeUserStream(const RawBytes &, bool goAhead);
    void sig_sendToSocket(const TelnetIacBytes &);
    void sig_relayGmcp(const GmcpMessage &);
    void sig_relayNaws(int, int);
    void sig_relayTermType(TelnetTermTypeBytes);

public slots:
    void slot_onSendToUser(const QString &data, bool goAhead);
    void slot_onAnalyzeUserStream(const TelnetIacBytes &);
    void slot_onConnected();
    void slot_onRelayEchoMode(bool);
    void slot_onGmcpToUser(const GmcpMessage &);
    void slot_onSendMSSPToUser(const TelnetMsspBytes &);
};
