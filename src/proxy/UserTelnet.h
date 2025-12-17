#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Signal2.h"
#include "AbstractTelnet.h"

#include <QByteArray>
#include <QObject>

struct NODISCARD UserTelnetOutputs
{
public:
    virtual ~UserTelnetOutputs();

public:
    void onAnalyzeUserStream(const RawBytes &bytes, const bool goAhead)
    {
        virt_onAnalyzeUserStream(bytes, goAhead);
    }
    void onSendToSocket(const TelnetIacBytes &bytes) { virt_onSendToSocket(bytes); }
    void onRelayGmcpFromUserToMud(const GmcpMessage &msg) { virt_onRelayGmcpFromUserToMud(msg); }
    void onRelayNawsFromUserToMud(const int width, const int height)
    {
        virt_onRelayNawsFromUserToMud(width, height);
    }
    void onRelayTermTypeFromUserToMud(const TelnetTermTypeBytes &bytes)
    {
        virt_onRelayTermTypeFromUserToMud(bytes);
    }
    void onGmcpModuleEnabled(const GmcpModuleTypeEnum type, const bool enabled)
    {
        virt_onGmcpModuleEnabled(type, enabled);
    }

private:
    virtual void virt_onAnalyzeUserStream(const RawBytes &, bool) = 0;
    virtual void virt_onSendToSocket(const TelnetIacBytes &) = 0;
    virtual void virt_onRelayGmcpFromUserToMud(const GmcpMessage &) = 0;
    virtual void virt_onRelayNawsFromUserToMud(int, int) = 0;
    virtual void virt_onRelayTermTypeFromUserToMud(const TelnetTermTypeBytes &) = 0;
    virtual void virt_onGmcpModuleEnabled(GmcpModuleTypeEnum, bool) = 0;
};

class NODISCARD UserTelnet final : public AbstractTelnet
{
private:
    /** modules for GMCP */
    struct NODISCARD GmcpData final
    {
        /** MMapper relevant modules and their version */
        GmcpModuleVersionList supported;
        /** All GMCP modules */
        GmcpModuleSet modules;
    } m_gmcp{};

private:
    UserTelnetOutputs &m_outputs;

public:
    explicit UserTelnet(UserTelnetOutputs &outputs);
    ~UserTelnet() final = default;

private:
    NODISCARD bool virt_isGmcpModuleEnabled(const GmcpModuleTypeEnum &name) const final;
    void virt_sendToMapper(const RawBytes &data, bool goAhead) final;
    void virt_receiveGmcpMessage(const GmcpMessage &) final;
    void virt_receiveTerminalType(const TelnetTermTypeBytes &) final;
    void virt_receiveWindowSize(int, int) final;
    void virt_sendRawData(const TelnetIacBytes &data) final;

private:
    void receiveGmcpModule(const GmcpModule &, bool);
    void resetGmcpModules();
    void sendSupportedGmcpModules();

public:
    void onAnalyzeUserStream(const TelnetIacBytes &);

public:
    void onSendToUser(const QString &data, bool goAhead);
    void onConnected();
    void onRelayEchoMode(bool);
    void onGmcpToUser(const GmcpMessage &);
    void onSendMSSPToUser(const TelnetMsspBytes &);
};
