#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"

#include <cstdint>

#include <glm/glm.hpp>

namespace bits {
template<typename T>
int bitCount(T x) noexcept = delete;
template<typename T>
int leastSignificantBit(T x) noexcept = delete;

template<>
NODISCARD inline int bitCount(uint8_t x) noexcept
{
    return glm::bitCount(x);
}
template<>
NODISCARD inline int bitCount(uint16_t x) noexcept
{
    return glm::bitCount(x);
}
template<>
NODISCARD inline int bitCount(uint32_t x) noexcept
{
    return glm::bitCount(x);
}
template<>
NODISCARD inline int bitCount(uint64_t x) noexcept
{
    return glm::bitCount(x);
}

template<>
NODISCARD inline int leastSignificantBit(uint8_t x) noexcept
{
    return glm::findLSB(x);
}

template<>
NODISCARD inline int leastSignificantBit(uint16_t x) noexcept
{
    return glm::findLSB(x);
}

template<>
NODISCARD inline int leastSignificantBit(uint32_t x) noexcept
{
    return glm::findLSB(x);
}

template<>
NODISCARD inline int leastSignificantBit(uint64_t x) noexcept
{
    return glm::findLSB(x);
}

} // namespace bits
