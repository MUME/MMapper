// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "JsonValue.h"

#include "JsonArray.h"
#include "JsonObj.h"

NODISCARD OptJsonArray JsonValue::getArray() const
{
    if (m_val.isArray()) {
        return OptJsonArray{m_val.toArray()};
    }
    return std::nullopt;
}

NODISCARD OptJsonBool JsonValue::getBool() const
{
    if (m_val.isBool()) {
        return OptJsonBool{m_val.toBool()};
    }
    return std::nullopt;
}

NODISCARD OptJsonInt JsonValue::getInt() const
{
    if (m_val.isDouble()) {
        return OptJsonInt{m_val.toInt()};
    }
    return std::nullopt;
}

NODISCARD OptJsonDouble JsonValue::getDouble() const
{
    if (m_val.isDouble()) {
        return OptJsonDouble{m_val.toDouble()};
    }
    return std::nullopt;
}

NODISCARD OptJsonObj JsonValue::getObject() const
{
    if (m_val.isObject()) {
        return OptJsonObj{m_val.toObject()};
    }
    return std::nullopt;
}

NODISCARD OptJsonString JsonValue::getString() const
{
    if (m_val.isString()) {
        return OptJsonString{m_val.toString()};
    }
    return std::nullopt;
}
