#pragma once
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

#ifndef MMAPPER_DOORFLAGS_H
#define MMAPPER_DOORFLAGS_H

#include <cstdint>

#include "../global/Flags.h"

// X(UPPER_CASE, lower_case, CamelCase, "Friendly Name")
#define X_FOREACH_DOOR_FLAG(X) \
    X(HIDDEN, hidden, Hidden, "Hidden") \
    X(NEED_KEY, need_key, NeedKey, "Need key") \
    X(NO_BLOCK, no_block, NoBlock, "No block") \
    X(NO_BREAK, no_break, NoBreak, "No break") \
    X(NO_PICK, no_pick, NoPick, "No pick") \
    X(DELAYED, delayed, Delayed, "Delayed") \
    X(CALLABLE, callable, Callable, "Callable") \
    X(KNOCKABLE, knockable, Knockable, "Knockable") \
    X(MAGIC, magic, Magic, "Magic") \
    X(ACTION, action, Action, "Action-controlled") \
    /* define door flags above */

enum class DoorFlag {
#define X_DECL_DOOR_FLAG(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    X_FOREACH_DOOR_FLAG(X_DECL_DOOR_FLAG)
#undef X_DECL_DOOR_FLAG
};

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int NUM_DOOR_FLAGS = X_FOREACH_DOOR_FLAG(X_COUNT);
#undef X_COUNT
DEFINE_ENUM_COUNT(DoorFlag, NUM_DOOR_FLAGS);

class DoorFlags final : public enums::Flags<DoorFlags, DoorFlag, uint16_t>
{
public:
    using Flags::Flags;

public:
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool is##CamelCase() const { return contains(DoorFlag::UPPER_CASE); }
    X_FOREACH_DOOR_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

public:
    // REVISIT: this name is different from the rest */
    inline bool needsKey() const { return isNeedKey(); }
};

inline constexpr const DoorFlags operator|(DoorFlag lhs, DoorFlag rhs) noexcept
{
    return DoorFlags{lhs} | DoorFlags{rhs};
}

#endif //MMAPPER_DOORFLAGS_H
