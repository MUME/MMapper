// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RAII.h"

#include <cassert>
#include <utility>

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
    if (!std::exchange(moved, true))
        ref = false;
}

RAIICallback::RAIICallback(RAIICallback &&rhs)
    : callback{std::exchange(rhs.callback, {})}
    , moved{std::exchange(rhs.moved, true)}
{}

RAIICallback::RAIICallback(RAIICallback::Callback &&callback)
    : callback{std::move(callback)}
{}

RAIICallback::~RAIICallback() noexcept(false)
{
    if (!moved)
        callback();
}
