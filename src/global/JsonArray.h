#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"
#include "JsonValue.h"

#include <optional>

#include <QJsonArray>

class JsonArrayIterator
{
private:
    QJsonArray::const_iterator m_iter;

public:
    explicit JsonArrayIterator(QJsonArray::const_iterator it)
        : m_iter(it)
    {}

    NODISCARD JsonValue operator*() const { return JsonValue{*m_iter}; }

    ALLOW_DISCARD JsonArrayIterator &operator++()
    {
        ++m_iter;
        return *this;
    }

    JsonArrayIterator operator++(int) = delete;

    NODISCARD bool operator==(const JsonArrayIterator &other) const
    {
        return m_iter == other.m_iter;
    }
    NODISCARD bool operator!=(const JsonArrayIterator &other) const
    {
        return m_iter != other.m_iter;
    }
};

class NODISCARD JsonArray final
{
public:
    using OptJsonArray = std::optional<JsonArray>;

private:
    QJsonArray m_arr;
    size_t m_beg = 0;
    size_t m_end = 0;

public:
    explicit JsonArray(QJsonArray arr)
        : m_arr(std::move(arr)) // CAUTION: using curly braces here calls initializer list ctor.
        , m_beg{0}
        , m_end{static_cast<size_t>(m_arr.size())}
    {}

public:
    NODISCARD bool empty() const { return m_beg == m_end; }
    NODISCARD size_t size() const { return m_end - m_beg; }
    NODISCARD size_t length() const { return size(); }

    NODISCARD const JsonValue front() const
    {
        if (empty()) {
            throw std::runtime_error("empty");
        }
        return JsonValue{m_arr.at(static_cast<int>(m_beg))};
    }
    NODISCARD const JsonValue back() const
    {
        if (empty()) {
            throw std::runtime_error("empty");
        }
        return JsonValue{m_arr.at(static_cast<int>(m_end - 1))};
    }

    NODISCARD JsonArrayIterator begin() const
    {
        return JsonArrayIterator(std::next(m_arr.begin(), static_cast<ptrdiff_t>(m_beg)));
    }

    NODISCARD JsonArrayIterator end() const
    {
        return JsonArrayIterator(std::next(m_arr.begin(), static_cast<ptrdiff_t>(m_end)));
    }

    NODISCARD JsonValue operator[](size_t index) const
    {
        if (index >= size()) {
            throw std::out_of_range("index out of range");
        }
        return JsonValue{m_arr.at(static_cast<int>(m_beg + index))};
    }
};
using OptJsonArray = JsonArray::OptJsonArray;
