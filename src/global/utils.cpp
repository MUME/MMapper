// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "utils.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <optional>

#include <QByteArray>
#include <qglobal.h>

namespace utils::details {

static_assert(!cpp17::IsValidCircularSize_v<0, bool>);
static_assert(!cpp17::IsValidCircularSize_v<1, bool>);
static_assert(cpp17::IsValidCircularSize_v<2, bool>);
static_assert(!cpp17::IsValidCircularSize_v<3, bool>);

static_assert(!cpp17::IsValidCircularSize_v<0, uint8_t>);
static_assert(!cpp17::IsValidCircularSize_v<1, uint8_t>);
static_assert(cpp17::IsValidCircularSize_v<2, uint8_t>);
static_assert(cpp17::IsValidCircularSize_v<3, uint8_t>);
static_assert(cpp17::IsValidCircularSize_v<255, uint8_t>);
static_assert(cpp17::IsValidCircularSize_v<256, uint8_t>);
static_assert(!cpp17::IsValidCircularSize_v<257, uint8_t>);

static_assert(cpp17::IsValidCircularSize_v<65536, uint16_t>);
static_assert(!cpp17::IsValidCircularSize_v<65537, uint16_t>);

static_assert(circular_increment<2, bool>(false) == true);
static_assert(circular_increment<2, bool>(true) == false);
static_assert(circular_decrement<2, bool>(false) == true);
static_assert(circular_decrement<2, bool>(true) == false);

static_assert(circular_increment<255, uint8_t>(254) == 0);
static_assert(circular_increment<256, uint8_t>(255) == 0);

static_assert(circular_increment<3, uint32_t>(0) == 1);
static_assert(circular_increment<3, uint32_t>(1) == 2);
static_assert(circular_increment<3, uint32_t>(2) == 0);
static_assert(circular_increment<3, uint32_t>(3) == 1); // doesn't throw; just wraps.

static_assert(circular_decrement<3, uint32_t>(0) == 2);
static_assert(circular_decrement<3, uint32_t>(1) == 0);
static_assert(circular_decrement<3, uint32_t>(2) == 1);
static_assert(circular_decrement<3, uint32_t>(3) == 2); // doesn't throw; just wraps.
static_assert(circular_decrement<3, uint32_t>(4) == 0); // doesn't throw; just wraps.

} // namespace utils::details

namespace utils::details::tests {

static_assert(!isBitMask<int32_t>());
static_assert(isBitMask<uint32_t>());

enum class NODISCARD NotBitmaskEnum { A = -1, B, C };
static_assert(!isBitMask<NotBitmaskEnum>());

enum class NODISCARD NotBitmask_intEnum : int { A = -1, B, C };
static_assert(!isBitMask<NotBitmask_intEnum>());

enum class NODISCARD NotBitmask_int8Enum : int8_t { A, B, C };
static_assert(!isBitMask<NotBitmask_int8Enum>());

enum class NODISCARD Bitmask_uint8Enum : uint8_t { A, B, C };
static_assert(isBitMask<Bitmask_uint8Enum>());

static_assert(!isBitMask<float>());
static_assert(!isBitMask<double>());

} // namespace utils::details::tests

namespace utils::tests {

static_assert(!isPowerOfTwo(0u) && !isAtLeastTwoBits(0u));
static_assert(isPowerOfTwo(1u) && !isAtLeastTwoBits(1u));
static_assert(isPowerOfTwo(2u) && !isAtLeastTwoBits(2u));
static_assert(!isPowerOfTwo(3u) && isAtLeastTwoBits(3u));
static_assert(isPowerOfTwo(4u) && !isAtLeastTwoBits(4u));
static_assert(!isPowerOfTwo(~0u) && isAtLeastTwoBits(~0u));
static constexpr const auto max = std::numeric_limits<uint32_t>::max();
static_assert(!isPowerOfTwo(max) && isAtLeastTwoBits(max));
static constexpr const auto topbit = max ^ (max >> 1);
static_assert(isPowerOfTwo(topbit) && !isAtLeastTwoBits(topbit));
} // namespace utils::tests

int utils::round_ftoi(const float f)
{
    using lim = std::numeric_limits<int>;
    assert(std::isfinite(f));
    const long l = std::lround(f);
    assert(isClamped<long>(l, lim::min(), lim::max()));
    return static_cast<int>(l);
}

std::optional<bool> utils::getEnvBool(const char *const key)
{
    if (!qEnvironmentVariableIsSet(key)) {
        return std::nullopt;
    }

    bool valid = false;
    const int value = qEnvironmentVariableIntValue(key, &valid);
    if (valid) {
        return value == 1;
    }

    QByteArray arr = qgetenv(key).toLower();
    if (arr == "true" || arr == "yes") {
        return true;
    } else if (arr == "false" || arr == "no") {
        return false;
    }

    return std::nullopt;
}

std::optional<int> utils::getEnvInt(const char *const key)
{
    if (!qEnvironmentVariableIsSet(key)) {
        return std::nullopt;
    }

    bool valid = false;
    const int value = qEnvironmentVariableIntValue(key, &valid);
    if (!valid) {
        return std::nullopt;
    }

    return value;
}

static_assert(!utils::equals(0.0, 1.0));
static_assert(utils::equals(1.0, 1.0));
static_assert(utils::equals(0.0, 0.0));
static_assert(utils::equals(0.0, -0.0));
