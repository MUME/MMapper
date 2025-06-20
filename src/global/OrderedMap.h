#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <cstddef>
#include <map>

template<typename K, typename V>
struct NODISCARD OrderedMap final
{
private:
    using Map = std::map<K, V>;

private:
    Map m_map;

public:
    NODISCARD const V *find(const K &key) const
    {
        if (auto it = m_map.find(key); it != m_map.end()) {
            return std::addressof(it->second);
        }
        return nullptr;
    }

    void set(const K &key, V val) { m_map[key] = std::move(val); }

    void erase(const K &key) { m_map.erase(key); }

public:
    NODISCARD size_t size() const { return m_map.size(); }
    NODISCARD bool empty() const { return m_map.empty(); }

public:
    NODISCARD auto begin() const { return m_map.begin(); }
    NODISCARD auto end() const { return m_map.end(); }

public:
    NODISCARD bool operator==(const OrderedMap &other) const { return m_map == other.m_map; }
    NODISCARD bool operator!=(const OrderedMap &other) const { return !operator==(other); }
};
