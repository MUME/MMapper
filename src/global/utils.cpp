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

namespace utils::details::tests {

static_assert(!isBitMask<int32_t>());
static_assert(isBitMask<uint32_t>());

enum class NotBitmask { A = -1, B, C };
static_assert(!isBitMask<NotBitmask>());

enum class NotBitmask_int : int { A = -1, B, C };
static_assert(!isBitMask<NotBitmask_int>());

enum class NotBitmask_int8 : int8_t { A, B, C };
static_assert(!isBitMask<NotBitmask_int8>());

enum class Bitmask_uint8 : uint8_t { A, B, C };
static_assert(isBitMask<Bitmask_uint8>());

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
    if (!qEnvironmentVariableIsSet(key))
        return std::nullopt;

    bool valid = false;
    const int value = qEnvironmentVariableIntValue(key, &valid);
    if (valid)
        return value == 1;

    QByteArray arr = qgetenv(key).toLower();
    if (arr == "true" || arr == "yes")
        return true;
    else if (arr == "false" || arr == "no")
        return false;

    return std::nullopt;
}

std::optional<int> utils::getEnvInt(const char *const key)
{
    if (!qEnvironmentVariableIsSet(key))
        return std::nullopt;

    bool valid = false;
    const int value = qEnvironmentVariableIntValue(key, &valid);
    if (!valid)
        return std::nullopt;

    return value;
}

static_assert(!utils::equals(0.0, 1.0));
static_assert(utils::equals(1.0, 1.0));
static_assert(utils::equals(0.0, 0.0));
static_assert(utils::equals(0.0, -0.0));
