#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "roomid.h"

#include <optional>
#include <set>

namespace detail {
template<typename Type_>
struct NODISCARD BasicRoomIdSet
{
private:
    using Type = Type_;
    using Set = std::set<Type>;

public:
    using ConstIterator = typename Set::const_iterator;

private:
    Set m_set;

public:
    BasicRoomIdSet() noexcept = default;
    explicit BasicRoomIdSet(const RoomId one) { insert(one); }

public:
    void clear() noexcept { m_set.clear(); }

public:
    NODISCARD ConstIterator cbegin() const { return m_set.begin(); }
    NODISCARD ConstIterator cend() const { return m_set.end(); }
    NODISCARD ConstIterator begin() const { return cbegin(); }
    NODISCARD ConstIterator end() const { return cend(); }
    NODISCARD size_t size() const { return m_set.size(); }
    NODISCARD bool empty() const noexcept { return m_set.empty(); }

public:
    NODISCARD bool contains(const Type id) const
    {
        if (auto it = m_set.find(id); it != m_set.end()) {
            return true;
        }
        return false;
    }

private:
    // O(|a| + |b|), instead of O(|a| * log |b|)
    NODISCARD static std::optional<Type> firstElementNotIn(const Set &a, const Set &b)
    {
        auto a_it = a.cbegin();
        const auto a_end = a.cend();
        auto b_it = b.cbegin();
        const auto b_end = b.cend();

        while (a_it != a_end) {
            const auto &x = *a_it;
            if (b_it == b_end) {
                return x;
            }
            const auto &y = *b_it;
            if (x < y) {
                return x;
            }
            if (!(y < x)) {
                ++a_it;
            }
            ++b_it;
        }
        return std::nullopt;
    }

public:
    // This probably isn't the most efficient set comparison function,
    // but it should work reasonably well.
    NODISCARD bool containsElementNotIn(const BasicRoomIdSet &other) const
    {
        if (this == &other) {
            return false;
        }

        if (false) {
            // O(N * log M)
            for (const Type id : m_set) {
                if (!other.contains(id)) {
                    return true;
                }
            }
            return false;
        }

        // O(N + M)
        return firstElementNotIn(m_set, other.m_set).has_value();
    }

    NODISCARD bool operator==(const BasicRoomIdSet &rhs) const { return m_set == rhs.m_set; }
    NODISCARD bool operator!=(const BasicRoomIdSet &rhs) const { return !operator==(rhs); }

public:
    void erase(const Type id) { m_set.erase(id); }
    void insert(const Type id) { m_set.insert(id); }

public:
    void insertAll(const BasicRoomIdSet &other) { m_set.insert(other.begin(), other.end()); }

public:
    NODISCARD Type first() const
    {
        if (empty()) {
            throw std::out_of_range("set is empty");
        }

        return *m_set.begin();
    }
    NODISCARD Type last() const
    {
        if (empty()) {
            throw std::out_of_range("set is empty");
        }

        return *m_set.rbegin();
    }
};
} // namespace detail

using RoomIdSet = detail::BasicRoomIdSet<RoomId>;
using ExternalRoomIdSet = detail::BasicRoomIdSet<ExternalRoomId>;

namespace test {
extern void testRoomIdSet();
} // namespace test
