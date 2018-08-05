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

#include "exit.h"

#include "../global/Flags.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/ExitFlags.h"

DoorName Exit::getDoorName() const
{
    assert(hasFields);
    return doorName;
}
bool Exit::hasDoorName() const
{
    assert(hasFields);
    return !doorName.isEmpty();
}
ExitFlags Exit::getExitFlags() const
{
    assert(hasFields);
    return exitFlags;
}
DoorFlags Exit::getDoorFlags() const
{
    assert(hasFields);
    return doorFlags;
}

void Exit::setDoorName(const DoorName name)
{
    assert(hasFields);
    doorName = name;
}
void Exit::setExitFlags(const ExitFlags flags)
{
    assert(hasFields);
    exitFlags = flags;
}
void Exit::setDoorFlags(const DoorFlags flags)
{
    assert(hasFields);
    doorFlags = flags;
}

void Exit::clearDoorName()
{
    setDoorName(DoorName{});
}
void Exit::clearExitFlags()
{
    setExitFlags(ExitFlags{});
}
void Exit::clearDoorFlags()
{
    setDoorFlags(DoorFlags{});
}

void Exit::removeDoorFlag(const DoorFlag flag)
{
    assert(hasFields);
    doorFlags &= ~DoorFlags{flag};
}

void Exit::orExitFlags(const ExitFlag flag)
{
    assert(hasFields);
    exitFlags |= flag;
}

void Exit::updateExit(ExitFlags flags)
{
    assert(hasFields);
    if (flags ^ exitFlags) {
        exitFlags |= flags;
    }
}

// bool Exit::exitXXX() const
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool Exit::exitIs##CamelCase() const { return getExitFlags().is##CamelCase(); }
X_FOREACH_EXIT_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

// bool Exit::doorIsXXX() const
#define X_DEFINE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool Exit::doorIs##CamelCase() const { return exitIsDoor() && getDoorFlags().is##CamelCase(); }
X_FOREACH_DOOR_FLAG(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

bool Exit::doorNeedsKey() const
{
    return isDoor() && getDoorFlags().needsKey();
}
