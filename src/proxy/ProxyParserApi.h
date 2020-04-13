#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <string_view>
#include <QByteArray>

#include "../global/WeakHandle.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/GmcpModule.h"

class Proxy;

// This is effectively a weak pointer to a virtual interface without the virtual;
// it basically only exists to avoid giving the parser private access to Parser.
class ProxyParserApi final
{
private:
    WeakHandle<Proxy> m_proxy;

public:
    explicit ProxyParserApi(WeakHandle<Proxy> proxy)
        : m_proxy(std::move(proxy))
    {}

public:
    bool isConnected() const;

public:
    void connectToMud() const;
    void disconnectFromMud() const;

public:
    void sendToMud(const QByteArray &msg) const;
    void sendToUser(const QByteArray &msg) const;

public:
    void sendToMud(const std::string_view &msg) const;
    void sendToUser(const std::string_view &msg) const;

public:
    void gmcpToMud(const GmcpMessage &msg) const;
    void gmcpToUser(const GmcpMessage &msg) const;
    bool isGmcpModuleEnabled(const GmcpModuleTypeEnum &module) const;
};
