// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ProxyParserApi.h"

#include "proxy.h"

// This reports the state of the outbound/toMud socket.
bool ProxyMudConnectionApi::isConnected() const
{
    return m_proxy.isConnected();
}

// This only affects the outbound/toMud socket.
void ProxyMudConnectionApi::connectToMud()
{
    m_proxy.connectToMud();
}

// This only affects the outbound/toMud socket.
void ProxyMudConnectionApi::disconnectFromMud()
{
    m_proxy.disconnectFromMud();
}

bool ProxyUserGmcpApi::isUserGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const
{
    return m_proxy.isUserGmcpModuleEnabled(mod);
}

void ProxyUserGmcpApi::gmcpToUser(const GmcpMessage &msg)
{
    m_proxy.gmcpToUser(msg);
}

bool ProxyMudGmcpApi::isMudGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const
{
    return m_proxy.isMudGmcpModuleEnabled(mod);
}

void ProxyMudGmcpApi::gmcpToMud(const GmcpMessage &msg)
{
    m_proxy.gmcpToMud(msg);
}
