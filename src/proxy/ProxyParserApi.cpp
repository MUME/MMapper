// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ProxyParserApi.h"

#include "proxy.h"

bool ProxyParserApi::isConnected() const
{
    bool result = false;
    m_proxy.acceptVisitor([&result](Proxy &proxy) { result = proxy.isConnected(); });
    return result;
}

void ProxyParserApi::connectToMud() const
{
    // FIXME: This breaks the design. Disconnect + reconnect should create a new proxy object.
    m_proxy.acceptVisitor([](Proxy &proxy) { proxy.connectToMud(); });
}

void ProxyParserApi::disconnectFromMud() const
{
    // FIXME: This breaks the design. Disconnect + reconnect should create a new proxy object.
    m_proxy.acceptVisitor([](Proxy &proxy) { proxy.disconnectFromMud(); });
}

void ProxyParserApi::gmcpToUser(const GmcpMessage &msg) const
{
    m_proxy.acceptVisitor([&msg](Proxy &proxy) { proxy.gmcpToUser(msg); });
}

bool ProxyParserApi::isGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const
{
    bool result = false;
    m_proxy.acceptVisitor(
        [&mod, &result](Proxy &proxy) { result = proxy.isGmcpModuleEnabled(mod); });
    return result;
}
