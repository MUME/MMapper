#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GmcpModule.h"

class GmcpMessage;
class Proxy;

class NODISCARD ProxyMudConnectionApi final
{
private:
    Proxy &m_proxy;

public:
    explicit ProxyMudConnectionApi(Proxy &proxy)
        : m_proxy{proxy}
    {}

public:
    NODISCARD bool isConnected() const;
    void connectToMud();
    void disconnectFromMud();
};

class NODISCARD ProxyUserGmcpApi final
{
private:
    Proxy &m_proxy;

public:
    explicit ProxyUserGmcpApi(Proxy &proxy)
        : m_proxy{proxy}
    {}

public:
    NODISCARD bool isUserGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const;
    void gmcpToUser(const GmcpMessage &msg);
};

class NODISCARD ProxyMudGmcpApi final
{
private:
    Proxy &m_proxy;

public:
    explicit ProxyMudGmcpApi(Proxy &proxy)
        : m_proxy{proxy}
    {}

public:
    NODISCARD bool isMudGmcpModuleEnabled(const GmcpModuleTypeEnum &mod) const;
    void gmcpToMud(const GmcpMessage &msg);
};
