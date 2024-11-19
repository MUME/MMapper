#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "room.h"

#include <cstddef>
#include <cstdlib>
#include <variant>

//
// X(UPPER_CASE, CamelCase, Type)
// SEP()
//
// NOTE: SEP() is required because of the use in std::variant<> declaration,
// which cannot accept trailing commas.
#define XFOREACH_ROOM_FIELD(X, SEP) \
    X(NAME, Name, RoomName) \
    SEP() \
    X(DESC, Description, RoomDesc) \
    SEP() \
    X(CONTENTS, Contents, RoomContents) \
    SEP() \
    X(NOTE, Note, RoomNote) \
    SEP() \
    X(MOB_FLAGS, MobFlags, RoomMobFlags) \
    SEP() \
    X(LOAD_FLAGS, LoadFlags, RoomLoadFlags) \
    SEP() \
    X(PORTABLE_TYPE, PortableType, RoomPortableEnum) \
    SEP() \
    X(LIGHT_TYPE, LightType, RoomLightEnum) \
    SEP() \
    X(ALIGN_TYPE, AlignType, RoomAlignEnum) \
    SEP() \
    X(RIDABLE_TYPE, RidableType, RoomRidableEnum) \
    SEP() \
    X(SUNDEATH_TYPE, SundeathType, RoomSundeathEnum) \
    SEP() \
    X(TERRAIN_TYPE, TerrainType, RoomTerrainEnum) \
    /* define room fields above */

#define X_DECL_ENUM(UPPER_CASE, CamelCase, Type) UPPER_CASE
#define X_COMMA() ,
enum class NODISCARD RoomFieldVariantOrderEnum { XFOREACH_ROOM_FIELD(X_DECL_ENUM, X_COMMA) };
#undef X_COMMA
#undef X_DECL_ENUM

class NODISCARD RoomFieldVariant final
{
private:
#define X_COMMA() ,
#define X_DECL_VAR_TYPES(UPPER_CASE, CamelCase, Type) Type
    std::variant<XFOREACH_ROOM_FIELD(X_DECL_VAR_TYPES, X_COMMA)> m_data;
#undef X_DECL_VAR_TYPES
#undef X_COMMA

public:
    RoomFieldVariant() = delete;
    DEFAULT_RULE_OF_5(RoomFieldVariant);

public:
#define X_NOP()
#define X_DEFINE_CTOR_AND_GETTER(UPPER_CASE, CamelCase, Type) \
    explicit RoomFieldVariant(Type val) \
        : m_data{std::move(val)} \
    {} \
    NODISCARD const Type &get##CamelCase() const \
    { \
        return std::get<Type>(m_data); \
    }
    XFOREACH_ROOM_FIELD(X_DEFINE_CTOR_AND_GETTER, X_NOP)
#undef X_DEFINE_CTOR_AND_GETTER
#undef X_NOP

public:
    NODISCARD RoomFieldEnum getType() const noexcept
    {
#define X_NOP()
#define X_CASE(UPPER_CASE, CamelCase, Type) \
    case static_cast<size_t>(RoomFieldVariantOrderEnum::UPPER_CASE): { \
        static_assert( \
            std::is_same_v<std::variant_alternative_t<static_cast<size_t>( \
                                                          RoomFieldVariantOrderEnum::UPPER_CASE), \
                                                      decltype(m_data)>, \
                           Type>); \
        return RoomFieldEnum::UPPER_CASE; \
    }
        switch (m_data.index()) {
            XFOREACH_ROOM_FIELD(X_CASE, X_NOP)
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
    NODISCARD bool operator==(const RoomFieldVariant &other) const
    {
        return m_data == other.m_data;
    }
    NODISCARD bool operator!=(const RoomFieldVariant &other) const
    {
        return !operator==(other);
    }
};
