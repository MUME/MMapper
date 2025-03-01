#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"
#include "JsonArray.h"
#include "JsonValue.h"

#include <cstddef>
#include <optional>
#include <utility>

#include <QJsonObject>

class JsonObjIterator
{
private:
    QJsonObject::const_iterator m_iter;

public:
    explicit JsonObjIterator(QJsonObject::const_iterator it)
        : m_iter(it)
    {}

    std::pair<JsonString, JsonValue> operator*() const
    {
        return {JsonString{m_iter.key()}, JsonValue{*m_iter}};
    }

    JsonString first() const { return JsonString{m_iter.key()}; }
    JsonValue second() const { return JsonValue(m_iter.value()); }

    JsonObjIterator &operator++()
    {
        ++m_iter;
        return *this;
    }

    JsonObjIterator operator++(int)
    {
        JsonObjIterator temp = *this;
        ++m_iter;
        return temp;
    }

    bool operator==(const JsonObjIterator &other) const { return m_iter == other.m_iter; }
    bool operator!=(const JsonObjIterator &other) const { return m_iter != other.m_iter; }
};

class NODISCARD JsonObj final
{
public:
    using OptJsonObj = std::optional<JsonObj>;

private:
    QJsonObject m_obj;
    size_t m_beg = 0;
    size_t m_end = 0;

public:
    explicit JsonObj(QJsonObject obj)
        : m_obj{std::move(obj)}
        , m_beg{0}
        , m_end{static_cast<size_t>(m_obj.size())}
    {}

    NODISCARD OptJsonArray getArray(const QString &name) const;
    NODISCARD OptJsonBool getBool(const QString &name) const;
    NODISCARD OptJsonInt getInt(const QString &name) const;
    NODISCARD OptJsonDouble getDouble(const QString &name) const;
    NODISCARD OptJsonNull getNull(const QString &name) const;
    NODISCARD OptJsonObj getObject(const QString &name) const;
    NODISCARD OptJsonString getString(const QString &name) const;

public:
    NODISCARD bool empty() const { return m_beg == m_end; }
    NODISCARD size_t size() const { return m_end - m_beg; }
    NODISCARD size_t length() const { return size(); }

    NODISCARD const JsonValue &front() const;
    NODISCARD const JsonValue &back() const;

    NODISCARD JsonObjIterator begin() const
    {
        return JsonObjIterator(std::next(m_obj.begin(), static_cast<int>(m_beg)));
    }

    NODISCARD JsonObjIterator end() const
    {
        return JsonObjIterator(std::next(m_obj.begin(), static_cast<int>(m_end)));
    }
};
using OptJsonObj = JsonObj::OptJsonObj;
