#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../global/RuleOf5.h"

struct Pair;
struct Value;

struct Vector final
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
    Base::const_iterator begin() const;
    Base::const_iterator end() const;

public:
    friend std::ostream &operator<<(std::ostream &os, const Vector &v);
};

struct Value final
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
    X(Vector, const Vector &, Vector)

#define DECL_ENUM(ValueType, RefType, CamelCase) CamelCase,
    enum class IndexEnum : uint8_t { Null, X_FOREACH_VALUE_TYPE(DECL_ENUM) };
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
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(m_value); }

#define DEFINE_CTOR_IS_GET(ValueType, RefType, CamelCase) \
    explicit Value(ValueType x) \
        : m_value(std::move(x)) \
    {} \
    bool is##CamelCase() const { return std::holds_alternative<ValueType>(m_value); } \
    RefType get##CamelCase() const { return std::get<ValueType>(m_value); }

    X_FOREACH_VALUE_TYPE(DEFINE_CTOR_IS_GET)

#undef DEFINE_CTOR_IS_GET

public:
    IndexEnum getType() const { return static_cast<IndexEnum>(m_value.index()); }

public:
    friend std::ostream &operator<<(std::ostream &os, const Value &value);
};

using OptValue = std::optional<Value>;

struct Pair final
{
public:
    Value car;
    const Pair *cdr = nullptr;

public:
    Pair(Value car, const Pair *const cdr)
        : car(std::move(car))
        , cdr(cdr)
    {}

public:
    Pair() = default;
    DEFAULT_RULE_OF_5(Pair);
};

Vector getAnyVectorReversed(const Pair *matched);
