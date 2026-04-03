#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "../global/enums.h"
#include "DoorFlags.h"
#include "ExitFlags.h"
#include "infomark.h"
#include "mmapper2room.h"

#include <vector>

namespace enums {
DECL_ENUM_GETTER2(RoomLightEnum, NUM_LIGHT_TYPES, getDefinedRoomLightTypes)
DECL_ENUM_GETTER2(RoomSundeathEnum, NUM_SUNDEATH_TYPES, getDefinedRoomSundeathTypes)
DECL_ENUM_GETTER2(RoomPortableEnum, NUM_PORTABLE_TYPES, getDefinedRoomPortbleTypes)
DECL_ENUM_GETTER2(RoomRidableEnum, NUM_RIDABLE_TYPES, getDefinedRoomRidableTypes)
DECL_ENUM_GETTER2(RoomAlignEnum, NUM_ALIGN_TYPES, getDefinedRoomAlignTypes)
DECL_ENUM_GETTER2(RoomTerrainEnum, NUM_ROOM_TERRAIN_TYPES, getAllTerrainTypes)
DECL_ENUM_GETTER2(RoomMobFlagEnum, NUM_ROOM_MOB_FLAGS, getAllMobFlags)
DECL_ENUM_GETTER2(RoomLoadFlagEnum, NUM_ROOM_LOAD_FLAGS, getAllLoadFlags)
DECL_ENUM_GETTER2(DoorFlagEnum, NUM_DOOR_FLAGS, getAllDoorFlags)
DECL_ENUM_GETTER2(ExitFlagEnum, NUM_EXIT_FLAGS, getAllExitFlags)
DECL_ENUM_GETTER2(InfomarkClassEnum, NUM_INFOMARK_CLASSES, getAllInfomarkClasses)
DECL_ENUM_GETTER2(InfomarkTypeEnum, NUM_INFOMARK_TYPES, getAllInfomarkTypes)
} // namespace enums

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

template<concepts::IsEnum E>
NODISCARD constexpr E getInvalidValue()
{
    if constexpr (concepts::IsEnum_with_UNDEFINED<E>) {
        return E::UNDEFINED;
    } else {
        // Note: You have to specialise getInvalidValue<E>() to use it with an enum
        // that doesn't have E::UNDEFINED.
        static_assert(std::is_void_v<E>);
        return E{};
    }
}

template<concepts::IsEnumFlags Flags>
constexpr auto getValidMask() -> typename Flags::bitmask_type
{
    static_assert(Flags::NUM_FLAGS <= 32);
    using Enum = typename Flags::Flag;
    using U = typename Flags::bitmask_type;
    Flags tmp;
    for (U n = 0; n < Flags::NUM_FLAGS; ++n) {
        const auto e = static_cast<Enum>(n);
        if (isValidEnumValue(e)) {
            tmp |= e;
        }
    }
    return static_cast<U>(tmp);
}

template<concepts::IsEnum Enum, typename Value>
    requires(std::is_same_v<Value, std::underlying_type_t<Enum>>)
NODISCARD constexpr Enum toEnum(const Value value)
{
    const auto result = static_cast<Enum>(value);
    if (!isValidEnumValue(result)) {
        return getInvalidValue<Enum>();
    }
    return result;
}

template<concepts::IsEnumFlags Flags, typename Value>
    requires(std::is_same_v<Value, typename Flags::bitmask_type>)
NODISCARD constexpr Flags bitmaskToFlags(const Value value)
{
    // we assume this is an enums::Flags
    using U = typename Flags::bitmask_type;
    static_assert(std::is_unsigned_v<Value> && sizeof(value) <= sizeof(uint32_t));
    static_assert(Flags::NUM_FLAGS > 0 && Flags::NUM_FLAGS <= 32);

    // flags can exclude bit values that are too high, but they don't exclude invalid enums
    // within the range of defined bits.
    MAYBE_UNUSED constexpr U naive_mask = static_cast<U>(
        static_cast<uint64_t>(U{1} << Flags::NUM_FLAGS) - U{1});
    constexpr U valid_mask = getValidMask<Flags>();

    return Flags{static_cast<U>(value & valid_mask)};
}

template<concepts::IsUnsignedEnum Enum>
NODISCARD constexpr Enum sanitizeEnum(const Enum value)
{
    return toEnum<Enum>(enums::to_underlying(value));
}

template<concepts::IsEnumFlags Flags>
NODISCARD constexpr Flags sanitizeFlags(const Flags flags)
{
    // note: Flags::underlying_type is distinct from the underlying type of the enum.
    using U = typename Flags::bitmask_type;
    return bitmaskToFlags<Flags>(static_cast<U>(flags));
}

} // namespace enums

namespace test {
extern void testMapEnums();
} // namespace test
