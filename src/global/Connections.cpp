// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Connections.h"

namespace mmqt {

Connections::~Connections()
{
    disconnectAll();
}

Connections &Connections::operator+=(QMetaObject::Connection c)
{
    m_connections.emplace_back(std::move(c));
    return *this;
}

void Connections::disconnectAll()
{
    for (const auto &c : m_connections) {
        QObject::disconnect(c);
    }
    m_connections.clear();
}

} // namespace mmqt
