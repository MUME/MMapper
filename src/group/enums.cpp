// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "enums.h"

#include "../global/enums.h"
#include "../global/tests.h"

namespace test {
void test_group_enums()
{
    for (auto x : DEFINED_CHARACTER_POSITIONS) {
        assert(x != CharacterPositionEnum::UNDEFINED);
    }
    for (auto x : DEFINED_CHARACTER_TYPES) {
        assert(x != CharacterTypeEnum::UNDEFINED);
    }
    TEST_ASSERT(DEFINED_CHARACTER_POSITIONS.size() == NUM_CHARACTER_POSITIONS - 1);
    TEST_ASSERT(DEFINED_CHARACTER_TYPES.size() == NUM_CHARACTER_TYPES - 1);
    TEST_ASSERT(ALL_CHARACTER_AFFECTS.size() == NUM_CHARACTER_AFFECTS);
}
} // namespace test
