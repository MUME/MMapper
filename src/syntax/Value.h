#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../map/DoorFlags.h"
#include "../map/ExitFlags.h"
#include "../map/infomark.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct Pair;
struct Value;

struct NODISCARD Vector final
{
    using Base = std::vector<Value>;

private:
    std::shared_ptr<const Base> m_vector;

public:
    explicit Vector(Base &&x);

public:
    Vector();
    DEFAULT_RULE_OF_5(Vector);

public:
    NODISCARD Base::const_iterator begin() const;
    NODISCARD Base::const_iterator end() const;

public:
    NODISCARD bool empty() const { return m_vector->empty(); }
    NODISCARD size_t size() const { return m_vector->size(); }
    // NOTE: at() throws if out of range.
    const Value &at(size_t pos) const { return m_vector->at(pos); }
    const Value &operator[](size_t pos) const { return at(pos); }

public:
    friend std::ostream &operator<<(std::ostream &os, const Vector &v);
};

struct NODISCARD Value final
{
public:
    // X(Type, RefType, CamelCase)
#define X_FOREACH_VALUE_TYPE(X) \
    X(bool, bool, Bool) \
    X(char, char, Char) \
    X(int32_t, int32_t, Int) \
    X(int64_t, int64_t, Long) \
    X(float, float, Float) \
    X(double, double, Double) \
    X(std::string, const std::string &, String) \
    X(Vector, const Vector &, Vector) \
    X(DoorFlagEnum, DoorFlagEnum, DoorFlag) \
    X(ExitFlagEnum, ExitFlagEnum, ExitFlag) \
    X(InfoMarkClassEnum, InfoMarkClassEnum, InfoMarkClass)

#define DECL_ENUM(ValueType, RefType, CamelCase) CamelCase,
    enum class NODISCARD IndexEnum : uint8_t { Null, X_FOREACH_VALUE_TYPE(DECL_ENUM) };
#undef DECL_ENUM

private:
#define TYPES(ValueType, RefType, CamelCase) , ValueType
    using Variant = std::variant<std::nullptr_t X_FOREACH_VALUE_TYPE(TYPES)>;
#undef TYPES

private:
    Variant m_value = nullptr;

public:
    Value() = default;
    DEFAULT_RULE_OF_5(Value);

public:
    template<typename T>
    explicit Value(T) = delete;

public:
    NODISCARD bool isNull() const
    {
        return std::holds_alternative<std::nullptr_t>(m_value);
    }

#define DEFINE_CTOR_IS_GET(ValueType, RefType, CamelCase) \
    explicit Value(ValueType x) \
        : m_value(std::move(x)) \
    {} \
    NODISCARD bool is##CamelCase() const \
    { \
        return std::holds_alternative<ValueType>(m_value); \
    } \
    NODISCARD RefType get##CamelCase() const \
    { \
        return std::get<ValueType>(m_value); \
    }

    X_FOREACH_VALUE_TYPE(DEFINE_CTOR_IS_GET)

#undef DEFINE_CTOR_IS_GET

public:
    NODISCARD IndexEnum getType() const
    {
        return static_cast<IndexEnum>(m_value.index());
    }

public:
    friend std::ostream &operator<<(std::ostream &os, const Value &value);
};

using OptValue = std::optional<Value>;

struct NODISCARD Pair final
{
public:
    Value car;
    const Pair *cdr = nullptr;

public:
    Pair(Value car_, const Pair *const cdr_)
        : car{std::move(car_)}
        , cdr{cdr_}
    {}

public:
    Pair() = default;
    DEFAULT_RULE_OF_5(Pair);
};

NODISCARD extern Vector getAnyVectorReversed(const Pair *matched);
