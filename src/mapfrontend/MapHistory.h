#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../map/Map.h"

#include <cstddef>
#include <deque>

class MapHistory
{
private:
    std::deque<Map> m_history;
    size_t m_max_size;
    bool m_capped;

public:
    MapHistory(bool capped = false, size_t max_size = 0);

public:
    void push(Map map);
    Map pop();
    void clear();

public:
    bool isEmpty() const;
};
