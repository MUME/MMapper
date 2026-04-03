// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "enums.h"

#include "../global/enums.h"
#include "../global/tests.h"
#include "DoorFlags.h"
#include "ExitFlags.h"
#include "mmapper2room.h"

#include <array>
#include <vector>

namespace { // anonymous
template<typename Enum>
    requires(concepts::IsUnsignedEnum<Enum> and concepts::IsEnum_with_UNDEFINED<Enum>)
void basicRoomEnumTest()
{
    static_assert(std::is_same_v<std::underlying_type_t<Enum>, uint8_t>);

    constexpr auto badAlignValue = static_cast<Enum>(255);
    static_assert(enums::isValidEnumValue(static_cast<Enum>(0)));
    static_assert(!enums::isValidEnumValue(badAlignValue));
    static_assert(enums::getInvalidValue<Enum>() == Enum::UNDEFINED);
    static_assert(enums::sanitizeEnum(badAlignValue) == Enum::UNDEFINED);
}

template<concepts::IsEnumFlags Flags>
void basicFlagsTest()
{
    using Enum = typename Flags::Flag;
    using U = typename Flags::bitmask_type;
    static_assert(concepts::IsUnsignedEnum<Enum>);
    static_assert(concepts::IsUnsignedIntegralNumeric<U>);

    constexpr auto flags = static_cast<Flags>(enums::getValidMask<Flags>());
    static_assert(enums::sanitizeFlags<Flags>(flags) == flags);
}

enum class NODISCARD TestFooEnum : uint8_t {
    ZERO = 0,
    TWO = 2,
};

struct NODISCARD TestFooFlags final : public enums::Flags<TestFooFlags, TestFooEnum, uint8_t, 3>
{
    using Flags::Flags;
};
} // namespace

namespace enums {

template<>
NODISCARD constexpr TestFooEnum getInvalidValue<TestFooEnum>()
{
    return TestFooEnum::TWO;
}

template<>
NODISCARD constexpr bool isValidEnumValue(const TestFooEnum value)
{
    switch (value) {
    case TestFooEnum::ZERO:
    case TestFooEnum::TWO:
        return true;
    default:
        return false;
    }
}

} // namespace enums

namespace { // anonymous

// demonstrates that illegal flags are removed
void testFooFlags()
{
    static_assert(enums::getInvalidValue<TestFooEnum>() == TestFooEnum::TWO);

    using Flags = TestFooFlags;
    using Enum = Flags::Flag;

    constexpr auto illegal_value = static_cast<Enum>(1);
    static_assert(enums::isValidEnumValue(TestFooEnum::ZERO));
    static_assert(!enums::isValidEnumValue(illegal_value));
    static_assert(enums::isValidEnumValue(TestFooEnum::TWO));
    static_assert(enums::sanitizeEnum(illegal_value) == TestFooEnum::TWO);

    constexpr auto valid = enums::getValidMask<Flags>();
    static_assert(valid == 0b101); // ZERO and TWO

    constexpr auto good_flags = static_cast<Flags>(valid);
    static_assert(good_flags == Flags{0b101});
    static_assert(good_flags.contains(TestFooEnum::ZERO));
    static_assert(!good_flags.contains(illegal_value));
    static_assert(good_flags.contains(TestFooEnum::TWO));

    constexpr auto bad_flags = good_flags | illegal_value;
    static_assert(bad_flags == Flags{0b111});
    static_assert(bad_flags.contains(illegal_value));
    static_assert(good_flags != bad_flags);

    static_assert(enums::sanitizeFlags<Flags>(bad_flags) == good_flags);
}

// demonstrates that illegal flags are removed without converting to the "invalid" value.
void testFooFlags2()
{
    static_assert(enums::getInvalidValue<TestFooEnum>() == TestFooEnum::TWO);

    using Flags = TestFooFlags;
    using Enum = Flags::Flag;

    constexpr auto illegal_value = static_cast<Enum>(1);
    static_assert(!enums::isValidEnumValue(illegal_value));
    static_assert(enums::sanitizeEnum(illegal_value) == TestFooEnum::TWO);

    constexpr auto good_flags = Flags{};
    static_assert(!good_flags.contains(TestFooEnum::ZERO));
    static_assert(!good_flags.contains(illegal_value));
    static_assert(!good_flags.contains(TestFooEnum::TWO));

    constexpr auto bad_flags = good_flags | illegal_value;
    static_assert(bad_flags == Flags{0b010});
    static_assert(bad_flags.contains(illegal_value));
    static_assert(good_flags != bad_flags);

    static_assert(enums::sanitizeFlags<Flags>(bad_flags) == good_flags);
}
} // namespace

void test::testMapEnums()
{
    basicRoomEnumTest<RoomAlignEnum>();
    basicRoomEnumTest<RoomLightEnum>();
    basicRoomEnumTest<RoomPortableEnum>();
    basicRoomEnumTest<RoomRidableEnum>();
    basicRoomEnumTest<RoomSundeathEnum>();
    basicRoomEnumTest<RoomTerrainEnum>();

    // These are based on enums where all the values are valid, so we can't use the basic enum test.
    basicFlagsTest<DoorFlags>();
    basicFlagsTest<ExitFlags>();
    basicFlagsTest<RoomMobFlags>();
    basicFlagsTest<RoomLoadFlags>();

    testFooFlags();
    testFooFlags2();
}
