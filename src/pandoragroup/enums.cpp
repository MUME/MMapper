// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "enums.h"

#include <array>
#include <vector>

#include "../global/enums.h"
#include "groupauthority.h"
#include "mmapper2character.h"

#define DEFINE_GETTER(E, N, name) \
    const std::array<E, N> &name() \
    { \
        static const auto things = ::enums::genEnumValues<E, N>(); \
        return things; \
    }
#define DEFINE_GETTER_DEFINED(E, N, name) \
    const std::vector<E> &name() \
    { \
        static const auto things = []() { \
            std::vector<E> result; \
            for (auto x : ::enums::genEnumValues<E, N>()) \
                if (x != E::UNDEFINED) \
                    result.emplace_back(x); \
            return result; \
        }(); \
        return things; \
    }

namespace enums {
DEFINE_GETTER_DEFINED(CharacterPosition, NUM_CHARACTER_POSITIONS, getAllCharacterPositions)
DEFINE_GETTER(GroupMetadata, NUM_GROUP_METADATA, getAllGroupMetadata)
DEFINE_GETTER(CharacterAffect, NUM_CHARACTER_AFFECTS, getAllCharacterAffects)
} // namespace enums

#undef DEFINE_GETTER
#undef DEFINE_GETTER_DEFINED
