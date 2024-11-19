// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RAII.h"

#include <cassert>
#include <utility>

#include <QtDebug>

RAIIBool::RAIIBool(bool &b)
    : ref{b}
{
    assert(!ref);
    ref = true;
}

RAIIBool::RAIIBool(RAIIBool &&rhs)
    : ref{rhs.ref}
    , moved{std::exchange(rhs.moved, true)}
{}

RAIIBool::~RAIIBool()
{
    if (!std::exchange(moved, true)) {
        ref = false;
    }
}

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
