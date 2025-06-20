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
    if (!satisfiesInvariants(id, dir)) {
        updateRawRoomRef(id, [dir](auto &r) { ::enforceInvariants(r.getExit(dir)); });
    }
}

void RawRooms::enforceInvariants(const RoomId id)
{
    if (!satisfiesInvariants(id)) {
        updateRawRoomRef(id, [](auto &r) { ::enforceInvariants(r); });
    }
}

bool RawRooms::satisfiesInvariants(const RoomId id, const ExitDirEnum dir) const
{
    return ::satisfiesInvariants(getRawRoomRef(id).getExit(dir));
}

bool RawRooms::satisfiesInvariants(const RoomId id) const
{
    return ::satisfiesInvariants(getRawRoomRef(id));
}
