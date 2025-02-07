#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Signal2.h"
#include "AbstractTelnet.h"

#include <QByteArray>
#include <QObject>

struct NODISCARD MsspTime final
{
    int year = -1;
    int month = -1;
    int day = -1;
    int hour = -1;
};

struct NODISCARD MudTelnetOutputs
{
    virtual ~MudTelnetOutputs();

public:
    void onAnalyzeMudStream(const RawBytes &bytes, const bool goAhead)
    {
        virt_onAnalyzeMudStream(bytes, goAhead);
    }
    void onSendToSocket(const TelnetIacBytes &bytes) { virt_onSendToSocket(bytes); }
    void onRelayEchoMode(const bool echo) { virt_onRelayEchoMode(echo); }
    void onRelayGmcpFromMudToUser(const GmcpMessage &msg) { virt_onRelayGmcpFromMudToUser(msg); }
    void onSendMSSPToUser(const TelnetMsspBytes &bytes) { virt_onSendMSSPToUser(bytes); }
    void onSendGameTimeToClock(const MsspTime &time) { virt_onSendGameTimeToClock(time); }
    void onTryCharLogin() { virt_onTryCharLogin(); }

private:
    virtual void virt_onAnalyzeMudStream(const RawBytes &, bool goAhead) = 0;
    virtual void virt_onSendToSocket(const TelnetIacBytes &) = 0;
    virtual void virt_onRelayEchoMode(bool) = 0;
    virtual void virt_onRelayGmcpFromMudToUser(const GmcpMessage &) = 0;
    virtual void virt_onSendMSSPToUser(const TelnetMsspBytes &) = 0;
    virtual void virt_onSendGameTimeToClock(const MsspTime &) = 0;
    virtual void virt_onTryCharLogin() = 0;
};

class NODISCARD MudTelnet final : public AbstractTelnet
{
private:
    MudTelnetOutputs &m_outputs;
    /** modules for GMCP */
    GmcpModuleSet m_gmcp;
    QString m_lineBuffer;
    bool m_receivedExternalDiscordHello = false;

public:
    explicit MudTelnet(MudTelnetOutputs &outputs);
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
    void submitOneLine(const QString &s);

private:
    void sendCoreSupports();

public:
    void onDisconnected();

public:
    void onAnalyzeMudStream(const TelnetIacBytes &);
    void onSubmitMpiToMud(const RawBytes &);
    void onSendToMud(const QString &);
    void onRelayNaws(int, int);
    void onRelayTermType(const TelnetTermTypeBytes &);
    void onGmcpToMud(const GmcpMessage &);
    void onLoginCredentials(const QString &, const QString &);
};
