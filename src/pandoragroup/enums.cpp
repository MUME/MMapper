/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "enums.h"

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
