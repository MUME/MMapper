// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Flags.h"

#include "tests.h"

#include <array>

namespace { // anonymous
enum class NODISCARD LetterEnum : uint8_t {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z
};
static_assert(static_cast<uint8_t>(LetterEnum::A) == 0);
static_assert(static_cast<uint8_t>(LetterEnum::Z) == 25);
struct NODISCARD LetterEnums final : public enums::Flags<LetterEnums, LetterEnum, uint32_t, 26>
{
    using Flags::Flags;
};
} // namespace

namespace test {
void testFlags()
{
    // CAUTION: This function behaves differently than you probably expect.
    //
    // This function returns the nth bit set, defined in order from least-significant
    // to most-significant bit that's currently set in the mask.

    LetterEnums letters = LetterEnums{LetterEnum::A} | LetterEnum::F | LetterEnum::Z;
    TEST_ASSERT(letters.size() == 3);
    TEST_ASSERT(letters[0u] == LetterEnum::A);
    TEST_ASSERT(letters[1u] == LetterEnum::F);
    TEST_ASSERT(letters[2u] == LetterEnum::Z);

    letters |= LetterEnum::D;
    TEST_ASSERT(letters.size() == 4);
    TEST_ASSERT(letters[0u] == LetterEnum::A);
    TEST_ASSERT(letters[1u] == LetterEnum::D);
    TEST_ASSERT(letters[2u] == LetterEnum::F);
    TEST_ASSERT(letters[3u] == LetterEnum::Z);

    size_t counted = 0;
    std::array<LetterEnum, 4> saw{};
    letters.for_each([&counted, &saw](const LetterEnum letter) {
        saw[counted] = letter;
        ++counted;
    });
    TEST_ASSERT(counted == 4);
    TEST_ASSERT(saw[0u] == LetterEnum::A);
    TEST_ASSERT(saw[1u] == LetterEnum::D);
    TEST_ASSERT(saw[2u] == LetterEnum::F);
    TEST_ASSERT(saw[3u] == LetterEnum::Z);
}
} // namespace test
