#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"

#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>

#include <immer/algorithm.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>

template<typename T>
struct NODISCARD ImmOrderedSet final
{
public:
    using Vector = immer::flex_vector<T>;
    using Type = T;
    using ConstIterator = typename Vector::const_iterator;

private:
    Vector m_vector;

public:
    ImmOrderedSet() = default;
    DEFAULT_RULE_OF_5(ImmOrderedSet);

public:
    explicit ImmOrderedSet(const std::set<T> &from)
        : m_vector{from.begin(), from.end()}
    {}

public:
    NODISCARD size_t size() const { return m_vector.size(); }
    NODISCARD bool empty() const { return m_vector.empty(); }

public:
    NODISCARD ConstIterator begin() const { return m_vector.begin(); }
    NODISCARD ConstIterator end() const { return m_vector.end(); }

public:
    void clear() noexcept { m_vector = Vector(); }

public:
    NODISCARD Type first() const
    {
        if (empty()) {
            throw std::out_of_range("set is empty");
        }
        return m_vector.front();
    }

    NODISCARD Type last() const
    {
        if (empty()) {
            throw std::out_of_range("set is empty");
        }
        return m_vector.back();
    }

    NODISCARD bool contains(const Type id) const
    {
        return std::binary_search(m_vector.begin(), m_vector.end(), id);
    }

public:
    void erase(const Type id)
    {
        const auto it = std::lower_bound(m_vector.begin(), m_vector.end(), id);
        if (it != m_vector.end() && *it == id) {
            const auto index = static_cast<size_t>(std::distance(m_vector.begin(), it));
            m_vector = std::move(m_vector).erase(index);
        }
    }

    void insert(const Type id)
    {
        const auto it = std::lower_bound(m_vector.begin(), m_vector.end(), id);
        if (it != m_vector.end() && *it == id) {
            return;
        }
        const auto index = static_cast<size_t>(std::distance(m_vector.begin(), it));
        if (index == m_vector.size()) {
            m_vector = std::move(m_vector).push_back(id);
            return;
        }
        m_vector = std::move(m_vector).insert(index, id);
    }

public:
    template<typename Callback>
    void for_each(Callback &&callback) const
    {
        immer::for_each_chunk(m_vector, [&callback](const Type *begin, const Type *end) {
            for (const Type *it = begin; it != end; ++it) {
                callback(*it);
            }
        });
    }

private:
    // O(|a| + |b|), for sorted vectors
    NODISCARD static std::optional<Type> firstElementNotIn(const Vector &a, const Vector &b)
    {
        auto a_it = a.begin();
        const auto a_end = a.end();
        auto b_it = b.begin();
        const auto b_end = b.end();

        while (a_it != a_end && b_it != b_end) {
            if (*a_it < *b_it) {
                return *a_it;
            } else if (*b_it < *a_it) {
                ++b_it;
            } else {
                ++a_it;
                ++b_it;
            }
        }

        if (a_it != a_end) {
            return *a_it;
        }

        return std::nullopt;
    }

public:
    // This probably isn't the most efficient set comparison function,
    // but it should work reasonably well.
    NODISCARD bool containsElementNotIn(const ImmOrderedSet &other) const
    {
        if (this == &other) {
            return false;
        }
        if (other.empty()) {
            return !this->empty();
        }
        if (this->empty()) {
            return false;
        }

        return firstElementNotIn(m_vector, other.m_vector).has_value();
    }

public:
    NODISCARD bool operator==(const ImmOrderedSet &rhs) const { return m_vector == rhs.m_vector; }
    NODISCARD bool operator!=(const ImmOrderedSet &rhs) const { return !operator==(rhs); }
};
