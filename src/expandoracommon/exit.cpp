// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
