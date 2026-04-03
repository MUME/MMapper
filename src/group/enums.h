#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/enums.h"
#include "mmapper2character.h"

namespace enums {
DECL_ENUM_GETTER2(CharacterPositionEnum, NUM_CHARACTER_POSITIONS, getDefinedCharacterPositions)
DECL_ENUM_GETTER2(CharacterTypeEnum, NUM_CHARACTER_TYPES, getDefinedCharacterTypes)
DECL_ENUM_GETTER2(CharacterAffectEnum, NUM_CHARACTER_AFFECTS, getAllCharacterAffects)
} // namespace enums

#define ALL_CHARACTER_AFFECTS ::enums::getAllCharacterAffects()
#define DEFINED_CHARACTER_POSITIONS ::enums::getDefinedCharacterPositions()
#define DEFINED_CHARACTER_TYPES ::enums::getDefinedCharacterTypes()

namespace test {
void test_group_enums();
}
