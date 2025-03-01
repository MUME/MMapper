// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "JsonObj.h"

#include "JsonArray.h"
#include "JsonValue.h"

NODISCARD OptJsonArray JsonObj::getArray(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isArray()) {
            return OptJsonArray{tmp.toArray()};
        }
    }
    return std::nullopt;
}

NODISCARD OptJsonBool JsonObj::getBool(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isBool()) {
            return OptJsonBool{tmp.toBool()};
        }
    }
    return std::nullopt;
}

NODISCARD OptJsonInt JsonObj::getInt(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isDouble()) {
            return OptJsonInt{tmp.toInt()};
        }
    }
    return std::nullopt;
}

NODISCARD OptJsonDouble JsonObj::getDouble(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isDouble()) {
            return OptJsonDouble{tmp.toDouble()};
        }
    }
    return std::nullopt;
}

NODISCARD OptJsonNull JsonObj::getNull(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isNull()) {
            return OptJsonNull{};
        }
    }
    return std::nullopt;
}

NODISCARD OptJsonObj JsonObj::getObject(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isObject()) {
            return OptJsonObj{tmp.toObject()};
        }
    }
    return std::nullopt;
}

NODISCARD OptJsonString JsonObj::getString(const QString &name) const
{
    if (m_obj.contains(name)) {
        if (auto &&tmp = m_obj.value(name); tmp.isString()) {
            return OptJsonString{tmp.toString()};
        }
    }
    return std::nullopt;
}
