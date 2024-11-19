// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "roomid.h"

#include "../global/AnsiOstream.h"

#include <ostream>

std::ostream &operator<<(std::ostream &os, const RoomId id)
{
    return os << "RoomId(" << id.value() << ")";
}

std::ostream &operator<<(std::ostream &os, const ExternalRoomId id)
{
    return os << "ExternalRoomId(" << id.value() << ")";
}

std::ostream &operator<<(std::ostream &os, const ServerRoomId id)
{
    return os << "ServerRoomId(" << id.value() << ")";
}

///

AnsiOstream &operator<<(AnsiOstream &os, const RoomId id)
{
    return os << "RoomId(" << id.value() << ")";
}

AnsiOstream &operator<<(AnsiOstream &os, const ExternalRoomId id)
{
    return os << "ExternalRoomId(" << id.value() << ")";
}

AnsiOstream &operator<<(AnsiOstream &os, const ServerRoomId id)
{
    return os << "ServerRoomId(" << id.value() << ")";
}
