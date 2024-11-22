#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ChangeMonitor.h"
#include "RuleOf5.h"
#include "utils.h"

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <QSlider>

template<int Digits_>
class NODISCARD FixedPoint final
{
private:
    ChangeMonitor m_changeMonitor;
    bool m_notifying = false;

public:
    static constexpr const int digits = Digits_;
    const int min;
    const int max;
    const int defaultValue;

private:
    int m_value = 0;

private:
    explicit FixedPoint(const int min_, const int max_, const int defaultValue_, const int value)
        : min{min_}
        , max{max_}
        , defaultValue{defaultValue_}
        , m_value{std::clamp(value, min_, max_)}
    {
        // set(value);
        static_assert(0 <= digits && digits < 6);
        if (min_ > max_)
            throw std::invalid_argument("min/max");
        if (defaultValue_ < min_ || defaultValue_ > max_)
            throw std::invalid_argument("defaultValue");
    }

public:
    explicit FixedPoint(const int min_, const int max_, const int defaultValue_)
        : FixedPoint(min_, max_, defaultValue_, defaultValue_)
    {}

    ~FixedPoint() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(FixedPoint);

public:
    void reset() { set(defaultValue); }
    void set(const int value)
    {
        if (m_notifying)
            throw std::runtime_error("recursion");

        const int newValue = std::clamp(value, min, max);
        if (m_value == newValue)
            return;

        struct NODISCARD NotificationGuard final
        {
            FixedPoint &m_self;
            NotificationGuard() = delete;
            DELETE_CTORS_AND_ASSIGN_OPS(NotificationGuard);
            explicit NotificationGuard(FixedPoint &self)
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

    void setFloat(const float f)
    {
        if (!std::isfinite(f))
            throw std::invalid_argument("f");
        return set(static_cast<int>(std::lround(f * std::pow(10.f, static_cast<float>(digits)))));
    }

    NODISCARD int get() const { return m_value; }
    NODISCARD float getFloat() const
    {
        return static_cast<float>(get()) * std::pow(10.f, -static_cast<float>(digits));
    }
    NODISCARD double getDouble() const
    {
        return static_cast<double>(get()) * std::pow(10.0, -static_cast<double>(digits));
    }

public:
    // NOTE: The clone does not inherit our change monitor!
    NODISCARD FixedPoint clone(int value) const
    {
        return FixedPoint<Digits_>{min, max, defaultValue, value};
    }

public:
    NODISCARD ChangeMonitor::CallbackLifetime registerChangeCallback(ChangeMonitor::Function callback)
    {
        return m_changeMonitor.registerChangeCallback(std::move(callback));
    }
};
