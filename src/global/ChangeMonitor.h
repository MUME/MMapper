#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Signal2.h"

#include <vector>

class NODISCARD ChangeMonitor final
{
public:
    using Sig = ::Signal2<>;
    using Function = Sig::Function;
    using Lifetime = Signal2Lifetime;
    Sig m_sig;

public:
    void registerChangeCallback(const Lifetime &lifetime, Function callback)
    {
        return m_sig.connect(lifetime, std::move(callback));
    }

public:
    void notifyAll() { m_sig.invoke(); }
};
