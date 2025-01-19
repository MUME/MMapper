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
#define X_FOREACH_ROOM_FIELD(X, SEP) \
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

#define DECL_ENUM(UPPER_CASE, CamelCase, Type) UPPER_CASE
#define COMMA() ,
enum class NODISCARD RoomFieldVariantOrderEnum { X_FOREACH_ROOM_FIELD(DECL_ENUM, COMMA) };
#undef COMMA
#undef DECL_ENUM

class NODISCARD RoomFieldVariant final
{
private:
#define COMMA() ,
#define DECL_VAR_TYPES(UPPER_CASE, CamelCase, Type) Type
    std::variant<X_FOREACH_ROOM_FIELD(DECL_VAR_TYPES, COMMA)> m_data;
#undef DECL_VAR_TYPES
#undef COMMA

public:
    RoomFieldVariant() = delete;
    DEFAULT_RULE_OF_5(RoomFieldVariant);

public:
#define NOP()
#define DEFINE_CTOR_AND_GETTER(UPPER_CASE, CamelCase, Type) \
    explicit RoomFieldVariant(Type val) \
        : m_data{std::move(val)} \
    {} \
    NODISCARD const Type &get##CamelCase() const \
    { \
        return std::get<Type>(m_data); \
    }
    X_FOREACH_ROOM_FIELD(DEFINE_CTOR_AND_GETTER, NOP)
#undef DEFINE_CTOR_AND_GETTER
#undef NOP

public:
    NODISCARD RoomFieldEnum getType() const noexcept
    {
#define NOP()
#define CASE(UPPER_CASE, CamelCase, Type) \
    case static_cast<size_t>(RoomFieldVariantOrderEnum::UPPER_CASE): { \
        static_assert( \
            std::is_same_v<std::variant_alternative_t<static_cast<size_t>( \
                                                          RoomFieldVariantOrderEnum::UPPER_CASE), \
                                                      decltype(m_data)>, \
                           Type>); \
        return RoomFieldEnum::UPPER_CASE; \
    }
        switch (const auto index = m_data.index()) {
            X_FOREACH_ROOM_FIELD(CASE, NOP)
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
    NODISCARD bool operator==(const RoomFieldVariant &other) const
    {
        return m_data == other.m_data;
    }
    NODISCARD bool operator!=(const RoomFieldVariant &other) const
    {
        return !operator==(other);
    }
};
