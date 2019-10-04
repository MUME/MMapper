#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <string_view>
#include <QByteArray>

#include "../global/WeakHandle.h"

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
    void connectToMud();
    void disconnectFromMud();

public:
    void sendToMud(const QByteArray &msg);
    void sendToUser(const QByteArray &msg);

public:
    void sendToMud(const std::string_view &msg);
    void sendToUser(const std::string_view &msg);
};
