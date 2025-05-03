// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "RawRooms.h"

#include "../global/logging.h"

void RawRooms::setExitFlags_safe(const RoomId id, const ExitDirEnum dir, const ExitFlags flags)
{
    setExitExitFlags(id, dir, flags);
    enforceInvariants(id, dir);
}

void RawRooms::enforceInvariants(const RoomId id, const ExitDirEnum dir)
{
    ::enforceInvariants(getRawRoomRef(id).getExit(dir));
}

void RawRooms::enforceInvariants(const RoomId id)
{
    ::enforceInvariants(getRawRoomRef(id));
}

bool RawRooms::satisfiesInvariants(const RoomId id, const ExitDirEnum dir) const
{
    return ::satisfiesInvariants(getRawRoomRef(id).getExit(dir));
}

bool RawRooms::satisfiesInvariants(const RoomId id) const
{
    return ::satisfiesInvariants(getRawRoomRef(id));
}
