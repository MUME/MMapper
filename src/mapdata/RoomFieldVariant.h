#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <QString>
#include <QtGlobal>

#include "../expandoracommon/room.h"
#include "../global/RuleOf5.h"

// X(UPPER_CASE, CamelCase, Type)
#define X_FOREACH_ROOM_FIELD(X) \
    X(NOTE, Note, RoomNote) \
    X(MOB_FLAGS, MobFlags, RoomMobFlags) \
    X(LOAD_FLAGS, LoadFlags, RoomLoadFlags) \
    X(PORTABLE_TYPE, PortableType, RoomPortableType) \
    X(LIGHT_TYPE, LightType, RoomLightType) \
    X(ALIGN_TYPE, AlignType, RoomAlignType) \
    X(RIDABLE_TYPE, RidableType, RoomRidableType) \
    X(SUNDEATH_TYPE, SundeathType, RoomSundeathType) \
    X(TERRAIN_TYPE, TerrainType, RoomTerrainType) \
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
    RoomField type_{};

public:
    RoomFieldVariant() = delete;

public:
    /* NOTE: Adding move support would be a pain for little gain.
     * You'd at least a boolean indicating that the object is in moved-from state. */
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
    inline RoomField getType() const { return type_; }

public:
#define X_DECLARE_ACCESSORS(UPPER_CASE, CamelCase, Type) Type get##CamelCase() const;
    X_FOREACH_ROOM_FIELD(X_DECLARE_ACCESSORS)
#undef X_DECLARE_ACCESSORS
};
