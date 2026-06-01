#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../map/Map.h"

#include <cstddef>
#include <deque>

class NODISCARD MapHistory final
{
private:
    std::deque<Map> m_history;
    size_t m_max_size;
    bool m_capped;

public:
    explicit MapHistory(bool capped = false, size_t max_size = 0);
    ~MapHistory() = default;
    MapHistory(const MapHistory &) = delete;
    MapHistory &operator=(const MapHistory &) = delete;

public:
    void push(Map map);
    NODISCARD Map pop();
    void clear();

public:
    NODISCARD bool isEmpty() const;
};
