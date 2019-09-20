#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Signal.h"

#include <vector>

class ChangeMonitor final : private ::Signal<>
{
public:
    using Base = ::Signal<>;
    using Function = Base::Function;
    using CallbackLifetime = Signal::SharedConnection;

public:
    NODISCARD CallbackLifetime registerChangeCallback(Base::Function callback)
    {
        return Base::connect(std::move(callback));
    }

    template<typename T>
    NODISCARD CallbackLifetime registerMemberFunctionChangeCallback(T &obj,
                                                                    Base::MemberFunctionPtr<T> pfn)
    {
        return Base::connectMember(obj, pfn);
    }

public:
    void notifyAll() { Base::invoke(); }
};

struct NODISCARD ConnectionSet final
{
private:
    std::vector<ChangeMonitor::CallbackLifetime> m_lifetimes;

public:
    ConnectionSet &operator+=(ChangeMonitor::CallbackLifetime lifetime)
    {
        m_lifetimes.emplace_back(lifetime);
        return *this;
    }
};
