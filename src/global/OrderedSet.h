#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"

#include <memory>
#include <set>
#include <stdexcept>

template<typename T>
struct NODISCARD OrderedSet final
{
public:
    using Set = std::set<T>;
    using ValueType = T;
    using ConstIterator = typename Set::const_iterator;

private:
    Set m_set;

public:
    OrderedSet() = default;
    DEFAULT_RULE_OF_5(OrderedSet);

public:
    explicit OrderedSet(const ValueType id) { insert(id); }
    explicit OrderedSet(const Set &other)
        : m_set{other}
    {}
    explicit OrderedSet(Set &&other)
        : m_set{std::move(other)}
    {}
    OrderedSet &operator=(const Set &other)
    {
        m_set = other;
        return *this;
    }
    OrderedSet &operator=(Set &&other)
    {
        m_set = std::move(other);
        return *this;
    }

public:
    NODISCARD size_t size() const { return m_set.size(); }
    NODISCARD bool empty() const { return m_set.empty(); }

public:
    NODISCARD ConstIterator begin() const { return m_set.begin(); }
    NODISCARD ConstIterator end() const { return m_set.end(); }

public:
    NODISCARD ValueType first() const
    {
        if (empty()) {
            throw std::runtime_error("empty set");
        }
        return *begin();
    }

    NODISCARD bool contains(const ValueType id) const { return m_set.find(id) != m_set.end(); }

public:
    void erase(const ValueType id) { m_set.erase(id); }
    void insert(const ValueType id) { m_set.insert(id); }

public:
    NODISCARD bool operator==(const OrderedSet &rhs) const { return m_set == rhs.m_set; }
    NODISCARD bool operator!=(const OrderedSet &rhs) const { return !(rhs == *this); }
};
