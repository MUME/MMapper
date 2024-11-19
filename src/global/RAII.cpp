// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RAII.h"

#include <utility>

#include <QtDebug>

RAIICallback::RAIICallback(RAIICallback &&rhs) noexcept
    : m_was_moved{std::exchange(rhs.m_was_moved, true)}
{
    static_assert(std::is_nothrow_swappable_v<Callback>);
    std::swap(m_callback, rhs.m_callback);
}

RAIICallback::RAIICallback(RAIICallback::Callback &&callback) noexcept
    : m_callback{std::move(callback)}
{
    static_assert(std::is_nothrow_move_constructible_v<Callback>);
}

RAIICallback::~RAIICallback()
{
    if (!m_was_moved) {
        try {
            m_callback();
        } catch (...) {
            qWarning() << "Exception in callback";
        }
    }
}
