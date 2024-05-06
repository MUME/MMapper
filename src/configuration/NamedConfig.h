#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/ChangeMonitor.h"

#include <cmath>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

template<typename T>
class NODISCARD NamedConfig final
{
private:
    std::string m_name;
    ChangeMonitor m_changeMonitor;
    T m_value = 0;
    bool m_notifying = false;

public:
    NamedConfig() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(NamedConfig);
    explicit NamedConfig(std::string name, T initialValue)
        : m_name{std::move(name)}
        , m_value{std::move(initialValue)}
    {}

public:
    NODISCARD const std::string &getName() { return m_name; }
    NODISCARD inline T get() const { return m_value; }
    void set(const T newValue)
    {
        if (m_notifying)
            throw std::runtime_error("recursion");

        if (m_value == newValue)
            return;

        struct NODISCARD NotificationGuard final
        {
            NamedConfig &m_self;
            NotificationGuard() = delete;
            DELETE_CTORS_AND_ASSIGN_OPS(NotificationGuard);
            explicit NotificationGuard(NamedConfig &self)
                : m_self{self}
            {
                if (m_self.m_notifying)
                    throw std::runtime_error("recursion");
                m_self.m_notifying = true;
            }

            ~NotificationGuard()
            {
                assert(m_self.m_notifying);
                m_self.m_notifying = false;
            }
        } notification_guard{*this};

        m_value = newValue;
        m_changeMonitor.notifyAll();
    }

    void clamp(const T lo, const T hi)
    {
        // don't try to call this for boolean or string.
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        if constexpr (std::is_floating_point_v<T>) {
            assert(std::isfinite(lo));
            assert(std::isfinite(hi));
        }
        assert(lo <= hi);
        set(std::clamp(get(), lo, hi));
    }

public:
    NODISCARD ChangeMonitor::CallbackLifetime registerChangeCallback(ChangeMonitor::Function callback)
    {
        return m_changeMonitor.registerChangeCallback(std::move(callback));
    }
};
