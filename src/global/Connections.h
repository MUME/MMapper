#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

#include <optional>
#include <vector>

#include <QMetaObject>
#include <QObject>

namespace mmqt {
class NODISCARD SingleConnection final
{
private:
    std::optional<QMetaObject::Connection> m_connection;

public:
    SingleConnection() = default;
    ~SingleConnection();
    SingleConnection(const SingleConnection &) = delete;
    SingleConnection &operator=(const SingleConnection &) = delete;

public:
    SingleConnection &operator=(QMetaObject::Connection c);
    void disconnect();
};

class NODISCARD Connections final
{
private:
    std::vector<QMetaObject::Connection> m_connections;

public:
    Connections() = default;
    ~Connections();
    Connections(const Connections &) = delete;
    Connections &operator=(const Connections &) = delete;

public:
    Connections &operator+=(QMetaObject::Connection c);
    void disconnectAll();
};

} // namespace mmqt
