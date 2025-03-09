// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "RoomHandle.h"

#include "Compare.h"
#include "Map.h"
#include "World.h"

const RawRoom &RoomHandle::getRaw() const
{
    return deref(m_room);
}

ExternalRawRoom RoomHandle::getRawCopyExternal() const
{
    const auto &raw = getRaw();
    return m_map.getWorld().convertToExternal(raw);
}

ExternalRoomId RoomHandle::getIdExternal() const
{
    return m_map.getWorld().convertToExternal(getId());
}

void RoomHandle::sanityCheck() const
{
    // This test probably isn't necessary, since only Map can call the nontrivial ctor.
    if (m_room != nullptr && !m_map.getWorld().hasRoom(getId())) {
        throw std::runtime_error("invalid RoomHandle");
    }
}

bool RoomHandle::exists() const
{
    return m_room != nullptr;
}

ServerRoomId RoomHandle::getServerId() const
{
    return deref(m_room).getServerId();
}

const Coordinate &RoomHandle::getPosition() const
{
    return deref(m_room).getPosition();
}

bool RoomHandle::isTemporary() const
{
    return deref(m_room).status == RoomStatusEnum::Temporary;
}

ExitDirFlags RoomHandle::computeExitDirections() const
{
    return ::computeExitDirections(deref(m_room));
}

ExitsFlagsType RoomHandle::computeExitsFlags() const
{
    return ::computeExitsFlags(deref(m_room));
}

bool RoomHandle::operator==(const RoomHandle &rhs) const
{
    return deref(m_room) == deref(rhs.m_room);
}

std::string RoomHandle::toStdStringUtf8() const
{
    return toStdStringUtf8_unsafe(*this);
}

#define X_DEFINE_GETTER(Type, Name, Init) \
    Type RoomHandle::get##Name() const \
    { \
        return m_map.getWorld().getRoom##Name(getId()); \
    }
XFOREACH_ROOM_PROPERTY(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

bool matches(const RawRoom &room, const ParseEvent &parseEvent)
{
    return ::compareWeakProps(room, parseEvent) != ComparisonResultEnum::DIFFERENT;
}
