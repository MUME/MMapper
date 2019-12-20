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

void ProxyParserApi::sendToMud(const QByteArray &msg) const
{
    if (msg.isEmpty())
        return;

    m_proxy.acceptVisitor([&msg](Proxy &proxy) { proxy.sendToMud(msg); });
}

void ProxyParserApi::sendToUser(const QByteArray &msg) const
{
    if (msg.isEmpty())
        return;

    m_proxy.acceptVisitor([&msg](Proxy &proxy) { proxy.sendToUser(msg); });
}

void ProxyParserApi::sendToMud(const std::string_view &msg) const
{
    if (msg.empty())
        return;

    sendToMud(QByteArray{msg.data(), static_cast<int>(msg.length())});
}

void ProxyParserApi::sendToUser(const std::string_view &msg) const
{
    if (msg.empty())
        return;

    sendToUser(QByteArray{msg.data(), static_cast<int>(msg.length())});
}
