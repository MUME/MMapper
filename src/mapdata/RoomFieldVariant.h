#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <QString>
#include <QtGlobal>

#include "../expandoracommon/room.h"
#include "../global/Array.h"
#include "../global/RuleOf5.h"

// X(UPPER_CASE, CamelCase, Type)
#define X_FOREACH_ROOM_FIELD(X) \
    X(NOTE, Note, RoomNote) \
    X(MOB_FLAGS, MobFlags, RoomMobFlags) \
    X(LOAD_FLAGS, LoadFlags, RoomLoadFlags) \
    X(PORTABLE_TYPE, PortableType, RoomPortableEnum) \
    X(LIGHT_TYPE, LightType, RoomLightEnum) \
    X(ALIGN_TYPE, AlignType, RoomAlignEnum) \
    X(RIDABLE_TYPE, RidableType, RoomRidableEnum) \
    X(SUNDEATH_TYPE, SundeathType, RoomSundeathEnum) \
    X(TERRAIN_TYPE, TerrainType, RoomTerrainEnum) \
    /* define room fields above */

template<typename T, typename... Rest>
struct max_align;

template<typename T>
struct max_align<T> : std::integral_constant<size_t, alignof(T)>
{};

template<typename T, typename U, typename... Rest>
struct max_align<T, U, Rest...>
    : std::integral_constant<size_t, std::max(max_align<T>::value, max_align<U, Rest...>::value)>
{};

template<typename T, typename... Rest>
struct max_size;

template<typename T>
struct max_size<T> : std::integral_constant<size_t, sizeof(T)>
{};

template<typename T, typename U, typename... Rest>
struct max_size<T, U, Rest...>
    : std::integral_constant<size_t, std::max(max_size<T>::value, max_size<U, Rest...>::value)>
{};

/* REVISIT: Replace this hacky variant.
 *
 * std::variant isn't available until c++17, but clang can't compile
 * the basic std::variant examples, so it's not really an option even on c++17.
 *
 * Consider using boost's templated variant, or one from another open source project?
 */
class RoomFieldVariant final
{
private:
#define X_DECLARE_TYPES(UPPER_CASE, CamelCase, Type) Type,
    // char type is added because of the trailing comma
#define TYPES X_FOREACH_ROOM_FIELD(X_DECLARE_TYPES) char
    static constexpr const size_t STORAGE_ALIGNMENT = max_align<TYPES>::value;
    static constexpr const size_t STORAGE_SIZE = max_size<TYPES>::value;
#undef TYPES
#undef X_DECLARE_TYPES
private:
    std::aligned_storage_t<STORAGE_SIZE, STORAGE_ALIGNMENT> storage_;
    static_assert(sizeof(storage_) >= STORAGE_SIZE);
    RoomFieldEnum type_ = RoomFieldEnum::NAME; // There is no good default value

public:
    RoomFieldVariant() = delete;

public:
    /* NOTE: Adding move support would be a pain for little gain.
     * You'd at least need a boolean indicating that the object is in moved-from state. */
    DELETE_MOVE_CTOR(RoomFieldVariant);
    DELETE_MOVE_ASSIGN_OP(RoomFieldVariant);

public:
    RoomFieldVariant(const RoomFieldVariant &rhs);
    RoomFieldVariant &operator=(const RoomFieldVariant &rhs);

public:
#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, CamelCase, Type) explicit RoomFieldVariant(Type);
    X_FOREACH_ROOM_FIELD(X_DECLARE_CONSTRUCTORS)
#undef X_DECLARE_CONSTRUCTORS
    ~RoomFieldVariant();

public:
    inline RoomFieldEnum getType() const { return type_; }

public:
#define X_DECLARE_ACCESSORS(UPPER_CASE, CamelCase, Type) Type get##CamelCase() const;
    X_FOREACH_ROOM_FIELD(X_DECLARE_ACCESSORS)
#undef X_DECLARE_ACCESSORS
};
