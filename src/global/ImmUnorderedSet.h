#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"

#include <memory>
#include <set>
#include <stdexcept>

#include <immer/set.hpp>

template<typename T>
struct NODISCARD ImmUnorderedSet final
{
public:
    using Set = immer::set<T>;
    using ValueType = T;

private:
    Set m_set;

public:
    ImmUnorderedSet() = default;
    DEFAULT_RULE_OF_5(ImmUnorderedSet);

public:
    explicit ImmUnorderedSet(const ValueType id) { insert(id); }
    explicit ImmUnorderedSet(const Set &other)
        : m_set{other}
    {}
    explicit ImmUnorderedSet(Set &&other)
        : m_set{std::move(other)}
    {}
    ImmUnorderedSet &operator=(const Set &other)
    {
        m_set = other;
        return *this;
    }
    ImmUnorderedSet &operator=(Set &&other)
    {
        m_set = std::move(other);
        return *this;
    }

public:
    NODISCARD size_t size() const { return m_set.size(); }
    NODISCARD bool empty() const { return m_set.empty(); }

public:
    NODISCARD auto begin() const { return m_set.begin(); }
    NODISCARD auto end() const { return m_set.end(); }

public:
    NODISCARD ValueType first() const
    {
        if (empty()) {
            throw std::runtime_error("empty set");
        }
        return *begin();
    }

    NODISCARD bool contains(const ValueType id) const { return m_set.find(id) != nullptr; }

public:
    void erase(const ValueType id) { m_set = std::move(m_set).erase(id); }
    void insert(const ValueType id) { m_set = std::move(m_set).insert(id); }

public:
    NODISCARD bool operator==(const ImmUnorderedSet &rhs) const { return m_set == rhs.m_set; }
    NODISCARD bool operator!=(const ImmUnorderedSet &rhs) const { return !(rhs == *this); }
};
