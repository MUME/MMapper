#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstddef>
#include <cstdlib>
#include <variant>
#include <QString>

#include "../global/RuleOf5.h"
#include "../global/TaggedString.h"
#include "DoorFlags.h"
#include "ExitFlags.h"

namespace tags {
struct NODISCARD DoorNameTag final
{};
} // namespace tags

using DoorName = TaggedString<tags::DoorNameTag>;

//
// X(UPPER_CASE, CamelCase)
// SEP()
//
// CamelCase is the same as the Type, so it's not repeated.
//
// NOTE: SEP() is required because of the use in std::variant<> declaration,
// which cannot accept trailing commas.
#define X_FOREACH_EXIT_FIELD(X, SEP) \
    X(DOOR_NAME, DoorName) \
    SEP() \
    X(EXIT_FLAGS, ExitFlags) \
    SEP() \
    X(DOOR_FLAGS, DoorFlags)

#define DECL_ENUM(UPPER_CASE, CamelCase) UPPER_CASE
#define COMMA() ,
enum class NODISCARD ExitFieldEnum { X_FOREACH_EXIT_FIELD(DECL_ENUM, COMMA) };
#undef COMMA
#undef DECL_ENUM

class NODISCARD ExitFieldVariant final
{
private:
#define COMMA() ,
#define DECL_VAR_TYPES(UPPER_CASE, CamelCase) CamelCase
    std::variant<X_FOREACH_EXIT_FIELD(DECL_VAR_TYPES, COMMA)> m_data;
#undef DECL_VAR_TYPES
#undef COMMA

public:
    ExitFieldVariant() = delete;
    DEFAULT_RULE_OF_5(ExitFieldVariant);

public:
#define NOP()
#define DEFINE_CTOR_AND_GETTER(UPPER_CASE, CamelCase) \
    explicit ExitFieldVariant(CamelCase val) \
        : m_data{std::move(val)} \
    {} \
    const CamelCase &get##CamelCase() const { return std::get<CamelCase>(m_data); }
    X_FOREACH_EXIT_FIELD(DEFINE_CTOR_AND_GETTER, NOP)
#undef DEFINE_CTOR_AND_GETTER
#undef NOP

public:
    NODISCARD ExitFieldEnum getType() const noexcept
    {
#define NOP()
#define CASE(UPPER_CASE, CamelCase) \
    case static_cast<size_t>(ExitFieldEnum::UPPER_CASE): { \
        static_assert( \
            std::is_same_v<std::variant_alternative_t<static_cast<size_t>(ExitFieldEnum::UPPER_CASE), \
                                                      decltype(m_data)>, \
                           CamelCase>); \
        return ExitFieldEnum::UPPER_CASE; \
    }
        switch (m_data.index()) {
            X_FOREACH_EXIT_FIELD(CASE, NOP)
        }
#undef CASE
#undef NOP

        std::abort(); /* crash */
    }

public:
    template<typename Visitor>
    void acceptVisitor(Visitor &&visitor) const
    {
        std::visit(std::forward<Visitor>(visitor), m_data);
    }

public:
    NODISCARD bool operator==(const ExitFieldVariant &other) const
    {
        return m_data == other.m_data;
    }
    NODISCARD bool operator!=(const ExitFieldVariant &other) const { return !operator==(other); }
};
