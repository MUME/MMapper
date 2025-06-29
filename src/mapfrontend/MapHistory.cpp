// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "MapHistory.h"

#include <stdexcept>

MapHistory::MapHistory(bool capped, size_t max_size)
    : m_max_size(max_size)
    , m_capped(capped)
{}

void MapHistory::push(Map map)
{
    m_history.push_back(std::move(map));
    if (m_capped && m_history.size() > m_max_size) {
        if (!m_history.empty()) {
            m_history.pop_front();
        }
    }
}

Map MapHistory::pop()
{
    if (m_history.empty()) {
        throw std::out_of_range("history is empty");
    }
    Map top_map = std::move(m_history.back());
    m_history.pop_back();
    return top_map;
}

void MapHistory::clear()
{
    m_history.clear();
}

bool MapHistory::isEmpty() const
{
    return m_history.empty();
}
