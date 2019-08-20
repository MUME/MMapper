// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "exit.h"

#include "../global/Flags.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/ExitFlags.h"

const DoorName &Exit::getDoorName() const
{
    return doorName;
}

bool Exit::hasDoorName() const
{
    return !doorName.isEmpty();
}

ExitFlags Exit::getExitFlags() const
{
    return exitFlags;
}

DoorFlags Exit::getDoorFlags() const
{
    return doorFlags;
}

void Exit::setDoorName(const DoorName name)
{
    doorName = name;
}

void Exit::setExitFlags(const ExitFlags flags)
{
    exitFlags = flags;
}

void Exit::setDoorFlags(const DoorFlags flags)
{
    doorFlags = flags;
}

void Exit::clearDoorName()
{
    setDoorName(DoorName{});
    assert(!hasDoorName());
}

void Exit::updateExit(ExitFlags flags)
{
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

bool Exit::operator==(const Exit &rhs) const
{
    return doorName == rhs.doorName && exitFlags == rhs.exitFlags && doorFlags == rhs.doorFlags
           && incoming == rhs.incoming && outgoing == rhs.outgoing;
}

bool Exit::operator!=(const Exit &rhs) const
{
    return !(rhs == *this);
}

template<typename T>
inline void maybeUpdate(T &x, const T &value)
{
    if (x != value)
        x = value;
}

void Exit::assignFrom(const Exit &rhs)
{
    maybeUpdate(doorName, rhs.doorName);
    doorFlags = rhs.doorFlags; // no allocation required
    exitFlags = rhs.exitFlags; // no allocation required
    maybeUpdate(incoming, rhs.incoming);
    maybeUpdate(outgoing, rhs.outgoing);
    assert(*this == rhs);
}
