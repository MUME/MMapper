#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"

#include <array>
#include <cstddef>
#include <cstdint>

class QByteArray;
class ProgressCounter;

namespace StorageUtils {
namespace Size {
namespace detail_types {
using C = char;
using UC = unsigned char;
using U = uint32_t;
} // namespace detail_types

NODISCARD static inline std::array<char, 4> encode(const uint32_t input)
{
    using namespace detail_types;
    const auto pack = [input](const size_t index) -> C {
        return static_cast<C>(static_cast<UC>(input >> (8 * (3 - index))));
    };
    return {pack(0), pack(1), pack(2), pack(3)};
}
NODISCARD static inline uint32_t decode(const std::array<char, 4> input)
{
    using namespace detail_types;
    const auto unpack = [input](const size_t index) -> U {
        return static_cast<U>(static_cast<UC>(input[index])) << (8 * (3 - index));
    };
    return unpack(0) | unpack(1) | unpack(2) | unpack(3);
}
} // namespace Size

namespace mmqt {
// decompression
NODISCARD QByteArray zlib_inflate(ProgressCounter &pc, const QByteArray &);
// compression
NODISCARD QByteArray zlib_deflate(ProgressCounter &pc, const QByteArray &, int level = -1);

// decodes 4-byte header (32-bit big-endian size) + zlib compression
NODISCARD QByteArray uncompress(ProgressCounter &pc, const QByteArray &input);
// encodes 4-byte header (32-bit big-endian size) + zlib compression
NODISCARD QByteArray compress(ProgressCounter &pc, const QByteArray &input);
} // namespace mmqt

} // namespace StorageUtils
