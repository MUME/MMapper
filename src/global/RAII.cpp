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

RAIICallback::RAIICallback(RAIICallback &&rhs)
    : m_callback{std::exchange(rhs.m_callback, {})}
    , m_was_moved{std::exchange(rhs.m_was_moved, true)}
{}

RAIICallback::RAIICallback(RAIICallback::Callback &&callback)
    : m_callback{std::move(callback)}
{}

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
