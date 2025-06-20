#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "macros.h"

#include <unordered_map>

#include <immer/map.hpp>

template<typename K, typename V>
struct NODISCARD ImmUnorderedMap final
{
    using Map = immer::map<K, V>;

private:
    Map m_map;

public:
    void init(const std::unordered_map<K, V> &map) { m_map = Map{map.begin(), map.end()}; }

    NODISCARD const V *find(const K &key) const { return m_map.find(key); }

    void set(const K &key, V val) { m_map = std::move(m_map).set(key, std::move(val)); }

    template<typename Callback>
    void update(const K &key, Callback &&callback)
    {
        m_map = std::move(m_map).update(key, [&callback](V val) -> V {
            callback(val);
            return val;
        });
    }

    void erase(const K &key) { m_map = std::move(m_map).erase(key); }

public:
    NODISCARD size_t size() const { return m_map.size(); }
    NODISCARD bool empty() const { return m_map.empty(); }

public:
    NODISCARD auto begin() const { return m_map.begin(); }
    NODISCARD auto end() const { return m_map.end(); }

public:
    NODISCARD bool operator==(const ImmUnorderedMap &other) const { return m_map == other.m_map; }
    NODISCARD bool operator!=(const ImmUnorderedMap &other) const { return !operator==(other); }
};
