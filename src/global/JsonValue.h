#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"

#include <cmath>
#include <cstdint>
#include <optional>

#include <QJsonValue>

class QString;
class JsonObj;
class JsonArray;

struct JsonNull
{};
using OptJsonNull = std::optional<JsonNull>;

using JsonBool = bool;
using OptJsonBool = std::optional<JsonBool>;

using JsonInt = int32_t;
using OptJsonInt = std::optional<JsonInt>;

using JsonDouble = double_t;
using OptJsonDouble = std::optional<JsonDouble>;

using JsonString = QString;
using OptJsonString = std::optional<JsonString>;

class NODISCARD JsonValue final
{
    using OptJsonArray = std::optional<JsonArray>;
    using OptJsonObj = std::optional<JsonObj>;

public:
    using OptJsonValue = std::optional<JsonValue>;

private:
    QJsonValue m_val;

public:
    explicit JsonValue(const QJsonValue &val)
        : m_val{std::move(val)}
    {}

    NODISCARD OptJsonArray getArray() const;
    NODISCARD OptJsonBool getBool() const;
    NODISCARD OptJsonInt getInt() const;
    NODISCARD OptJsonDouble getDouble() const;
    NODISCARD OptJsonObj getObject() const;
    NODISCARD OptJsonString getString() const;
};
using OptJsonValue = JsonValue::OptJsonValue;
