#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/TaggedString.h"
#include "DoorFlags.h"
#include "ExitFlags.h"

#include <cstddef>
#include <cstdlib>
#include <variant>

#include <QString>

namespace tags {
struct NODISCARD DoorNameTag final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

using DoorName = TaggedBoxedStringUtf8<tags::DoorNameTag>;

//
// X(UPPER_CASE, CamelCase)
// SEP()
//
// CamelCase is the same as the Type, so it's not repeated.
//
// NOTE: SEP() is required because of the use in std::variant<> declaration,
// which cannot accept trailing commas.
#define XFOREACH_EXIT_FIELD(X, SEP) \
    X(DOOR_NAME, DoorName) \
    SEP() \
    X(EXIT_FLAGS, ExitFlags) \
    SEP() \
    X(DOOR_FLAGS, DoorFlags)

#define X_DECL_ENUM(UPPER_CASE, CamelCase) UPPER_CASE
#define X_COMMA() ,
enum class NODISCARD ExitFieldEnum { XFOREACH_EXIT_FIELD(X_DECL_ENUM, X_COMMA) };
#undef X_COMMA
#undef X_DECL_ENUM

class NODISCARD ExitFieldVariant final
{
private:
#define X_COMMA() ,
#define X_DECL_VAR_TYPES(UPPER_CASE, CamelCase) CamelCase
    std::variant<XFOREACH_EXIT_FIELD(X_DECL_VAR_TYPES, X_COMMA)> m_data;
#undef X_DECL_VAR_TYPES
#undef X_COMMA

public:
    ExitFieldVariant() = delete;
    DEFAULT_RULE_OF_5(ExitFieldVariant);

public:
#define X_NOP()
#define X_DEFINE_CTOR_AND_GETTER(UPPER_CASE, CamelCase) \
    explicit ExitFieldVariant(CamelCase val) \
        : m_data{std::move(val)} \
    {} \
    const CamelCase &get##CamelCase() const { return std::get<CamelCase>(m_data); }
    XFOREACH_EXIT_FIELD(X_DEFINE_CTOR_AND_GETTER, X_NOP)
#undef X_DEFINE_CTOR_AND_GETTER
#undef X_NOP

public:
    NODISCARD ExitFieldEnum getType() const noexcept
    {
#define X_NOP()
#define X_CASE(UPPER_CASE, CamelCase) \
    case static_cast<size_t>(ExitFieldEnum::UPPER_CASE): { \
        static_assert( \
            std::is_same_v<std::variant_alternative_t<static_cast<size_t>(ExitFieldEnum::UPPER_CASE), \
                                                      decltype(m_data)>, \
                           CamelCase>); \
        return ExitFieldEnum::UPPER_CASE; \
    }
        switch (m_data.index()) {
            XFOREACH_EXIT_FIELD(X_CASE, X_NOP)
        }
#undef X_CASE
#undef X_NOP

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

NODISCARD extern DoorName makeDoorName(std::string doorName);
namespace mmqt {
NODISCARD extern DoorName makeDoorName(const QString &doorName);
}
