#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "DoorFlags.h"
#include "ExitFlags.h"
#include "infomark.h"
#include "mmapper2room.h"

#include <vector>

#define X_DECL_GETTER(E, N, name) const MMapper::Array<E, N> &name();
#define X_DECL_GETTER_DEFINED(E, N, name) const std::vector<E> &name();

namespace enums {
X_DECL_GETTER_DEFINED(RoomLightEnum, NUM_LIGHT_TYPES, getDefinedRoomLightTypes)
X_DECL_GETTER_DEFINED(RoomSundeathEnum, NUM_SUNDEATH_TYPES, getDefinedRoomSundeathTypes)
X_DECL_GETTER_DEFINED(RoomPortableEnum, NUM_PORTABLE_TYPES, getDefinedRoomPortbleTypes)
X_DECL_GETTER_DEFINED(RoomRidableEnum, NUM_RIDABLE_TYPES, getDefinedRoomRidableTypes)
X_DECL_GETTER_DEFINED(RoomAlignEnum, NUM_ALIGN_TYPES, getDefinedRoomAlignTypes)
X_DECL_GETTER(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES, getAllTerrainTypes)
X_DECL_GETTER(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
X_DECL_GETTER(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
X_DECL_GETTER(DoorFlagEnum, NUM_DOOR_FLAGS, getAllDoorFlags)
X_DECL_GETTER(ExitFlagEnum, NUM_EXIT_FLAGS, getAllExitFlags)
X_DECL_GETTER(InfomarkClassEnum, NUM_INFOMARK_CLASSES, getAllInfomarkClasses)
X_DECL_GETTER(InfomarkTypeEnum, NUM_INFOMARK_TYPES, getAllInfomarkTypes)
} // namespace enums

#undef X_DECL_GETTER
#undef X_DECL_GETTER_DEFINED

#define ALL_TERRAIN_TYPES ::enums::getAllTerrainTypes()
#define ALL_DOOR_FLAGS ::enums::getAllDoorFlags()
#define ALL_EXIT_FLAGS ::enums::getAllExitFlags()
#define ALL_INFOMARK_CLASSES ::enums::getAllInfomarkClasses()
#define ALL_INFOMARK_TYPES ::enums::getAllInfomarkTypes()
#define ALL_MOB_FLAGS ::enums::getAllMobFlags()
#define ALL_LOAD_FLAGS ::enums::getAllLoadFlags()

/* NOTE: These are actually used; but they're hidden in macros as DEFINED_ROOM_##X##_TYPES */
#define DEFINED_ROOM_LIGHT_TYPES ::enums::getDefinedRoomLightTypes()
#define DEFINED_ROOM_SUNDEATH_TYPES ::enums::getDefinedRoomSundeathTypes()
#define DEFINED_ROOM_PORTABLE_TYPES ::enums::getDefinedRoomPortbleTypes()
#define DEFINED_ROOM_RIDABLE_TYPES ::enums::getDefinedRoomRidableTypes()
#define DEFINED_ROOM_ALIGN_TYPES ::enums::getDefinedRoomAlignTypes()

namespace enums {

template<typename T>
NODISCARD constexpr bool isValidEnumValue(T value);

template<>
constexpr bool isValidEnumValue(const DoorFlagEnum value)
{
#define X_CASE(_UPPER_CASE, _lower_case, _CamelCase, _friendlyName) case DoorFlagEnum::_UPPER_CASE:
    switch (value) {
        XFOREACH_DOOR_FLAG(X_CASE)
        return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const ExitFlagEnum value)
{
#define X_CASE(_UPPER_CASE, _lower_case, _CamelCase, _friendlyName) case ExitFlagEnum::_UPPER_CASE:
    switch (value) {
        XFOREACH_EXIT_FLAG(X_CASE)
        return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomLoadFlagEnum value)
{
#define X_CASE(_x) case RoomLoadFlagEnum::_x:
    switch (value) {
        XFOREACH_ROOM_LOAD_FLAG(X_CASE)
        return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomMobFlagEnum value)
{
#define X_CASE(_x) case RoomMobFlagEnum::_x:
    switch (value) {
        XFOREACH_ROOM_MOB_FLAG(X_CASE)
        return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomAlignEnum value)
{
#define X_CASE(_x) case RoomAlignEnum::_x:
    switch (value) {
        XFOREACH_RoomAlignEnum(X_CASE) return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomLightEnum value)
{
#define X_CASE(_x) case RoomLightEnum::_x:
    switch (value) {
        XFOREACH_RoomLightEnum(X_CASE) return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomPortableEnum value)
{
#define X_CASE(_x) case RoomPortableEnum::_x:
    switch (value) {
        XFOREACH_RoomPortableEnum(X_CASE) return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomRidableEnum value)
{
#define X_CASE(_x) case RoomRidableEnum::_x:
    switch (value) {
        XFOREACH_RoomRidableEnum(X_CASE) return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomSundeathEnum value)
{
#define X_CASE(_x) case RoomSundeathEnum::_x:
    switch (value) {
        XFOREACH_RoomSundeathEnum(X_CASE) return true;
    default:
        return false;
    }
#undef X_CASE
}

template<>
constexpr bool isValidEnumValue(const RoomTerrainEnum value)
{
#define X_CASE(_x) case RoomTerrainEnum::_x:
    switch (value) {
        XFOREACH_RoomTerrainEnum(X_CASE) return true;
    default:
        return false;
    }
#undef X_CASE
}

template<typename E>
NODISCARD constexpr E getInvalidValue();
template<>
constexpr RoomAlignEnum getInvalidValue()
{
    return RoomAlignEnum::UNDEFINED;
}
template<>
constexpr RoomLightEnum getInvalidValue()
{
    return RoomLightEnum::UNDEFINED;
}
template<>
constexpr RoomPortableEnum getInvalidValue()
{
    return RoomPortableEnum::UNDEFINED;
}
template<>
constexpr RoomRidableEnum getInvalidValue()
{
    return RoomRidableEnum::UNDEFINED;
}
template<>
constexpr RoomSundeathEnum getInvalidValue()
{
    return RoomSundeathEnum::UNDEFINED;
}
template<>
constexpr RoomTerrainEnum getInvalidValue()
{
    return RoomTerrainEnum::UNDEFINED;
}

template<typename Flags>
constexpr auto getValidMask() -> typename Flags::underlying_type
{
    static_assert(Flags::NUM_FLAGS <= 32);
    using Enum = typename Flags::Flag;
    using U = typename Flags::underlying_type;
    Flags tmp;
    for (U n = 0; n < Flags::NUM_FLAGS; ++n) {
        const auto e = static_cast<Enum>(n);
        if (isValidEnumValue(e)) {
            tmp |= e;
        }
    }
    return static_cast<U>(tmp);
}

template<typename Enum, typename Value>
NODISCARD constexpr Enum toEnum(const Value value)
{
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<Value, std::underlying_type_t<Enum>>);
    const auto result = static_cast<Enum>(value);
    if (!isValidEnumValue(result)) {
        return getInvalidValue<Enum>();
    }
    return result;
}

template<typename Flags, typename Value>
NODISCARD constexpr Flags bitmaskToFlags(const Value value)
{
    // we assume this is an enums::Flags
    using U = typename Flags::underlying_type;
    static_assert(std::is_same_v<Value, U>);

    using Enum = typename Flags::Flag;
    static_assert(std::is_enum_v<Enum>);

    static_assert(std::is_unsigned_v<Value> && sizeof(value) <= sizeof(uint32_t));
    static_assert(Flags::NUM_FLAGS > 0 && Flags::NUM_FLAGS <= 32);

    // flags can exclude bit values that are too high, but they don't exclude invalid enums
    // within the range of defined bits.
    MAYBE_UNUSED constexpr U naive_mask = static_cast<U>(
        static_cast<uint64_t>(U{1} << Flags::NUM_FLAGS) - U{1});
    constexpr U valid_mask = getValidMask<Flags>();

    return Flags{static_cast<U>(value & valid_mask)};
}

template<typename Enum>
NODISCARD constexpr Enum sanitizeEnum(const Enum value)
{
    using U = std::underlying_type_t<Enum>;
    static_assert(std::is_unsigned_v<U>);
    return toEnum<Enum>(static_cast<U>(value));
}

template<typename Flags>
NODISCARD constexpr Flags sanitizeFlags(const Flags flags)
{
    using U = typename Flags::underlying_type;
    static_assert(std::is_unsigned_v<U>);
    return bitmaskToFlags<Flags>(static_cast<U>(flags));
}

} // namespace enums

namespace test {
extern void testMapEnums();
} // namespace test
