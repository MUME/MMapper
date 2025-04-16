#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "mmapper2character.h"

#include <vector>

#define DECL_GETTER(E, N, name) const MMapper::Array<E, N> &name();
#define DECL_GETTER_DEFINED(E, N, name) const std::vector<E> &name();

namespace enums {
DECL_GETTER_DEFINED(CharacterPositionEnum, NUM_CHARACTER_POSITIONS, getAllCharacterPositions)
DECL_GETTER_DEFINED(CharacterTypeEnum, NUM_CHARACTER_TYPES, getAllCharacterTypes)
DECL_GETTER(CharacterAffectEnum, NUM_CHARACTER_AFFECTS, getAllCharacterAffects)
} // namespace enums

#undef DECL_GETTER
#undef DECL_GETTER_DEFINED

#define ALL_CHARACTER_AFFECTS ::enums::getAllCharacterAffects()

/* NOTE: These are actually used; but they're hidden in macros as DEFINED_CHARACTER_##X##_TYPES */
#define DEFINED_CHARACTER_POSITIONS ::enums::getDefinedCharacterPositions()
#define DEFINED_CHARACTER_TYPES ::enums::getDefinedCharacterTypes()
