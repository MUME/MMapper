// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "connectionselection.h"

#include "../configuration/NamedConfig.h"
#include "../map/coordinate.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapfrontend/mapfrontend.h"

#include <cassert>
#include <utility>

ConnectionSelection::ConnectionSelection(Badge<ConnectionSelection>, MapFrontend &map)
    : m_map{map}
{
    for (const auto &x : m_connectionDescriptor) {
        assert(x.room == std::nullopt);
    }
}

ConnectionSelection::ConnectionSelection(Badge<ConnectionSelection>,
                                         MapFrontend &map,
                                         const MouseSel &sel)
    : m_map{map}
{
    for (const auto &x : m_connectionDescriptor) {
        assert(x.room == std::nullopt);
    }

    const Coordinate c = sel.getCoordinate();
    if (const auto room = m_map.findRoomHandle(c)) {
        this->receiveRoom(deref(room));
    }
    m_connectionDescriptor[0].direction = computeDirection(sel.pos);
}

ConnectionSelection::~ConnectionSelection() = default;

bool ConnectionSelection::isValid() const
{
    for (const auto &x : m_connectionDescriptor) {
        if (x.room == std::nullopt) {
            return false;
        }
    }
    return true;
}

// \NNNNNNNN/
// W\NNNN--/E
// WW\NN|UU|E
// WWW\-|UU|E
// WWW|CC--EE
// WW--CC|EEE
// W|DD|-\EEE
// W|DD|SS\EE
// W/--SSSS\E
// /SSSSSSSS\.
ExitDirEnum ConnectionSelection::computeDirection(const Coordinate2f &c)
{
    const glm::vec2 pos = glm::fract(c.to_vec2());
    const glm::vec2 upCenter{0.75f, 0.75f};
    const glm::vec2 downCenter{0.25f, 0.25f};
    const glm::vec2 actualCenter{0.5f, 0.5f};
    const float upDownRadius = 0.15f;
    const float centerRadius = 0.15f;

    if (glm::distance(pos, upCenter) <= upDownRadius) {
        return ExitDirEnum::UP;
    } else if (glm::distance(pos, downCenter) <= upDownRadius) {
        return ExitDirEnum::DOWN;
    } else if (glm::distance(pos, actualCenter) <= centerRadius) {
        return ExitDirEnum::UNKNOWN;
    }

    const bool ne = pos.x >= 1.f - pos.y;
    const bool nw = pos.x <= pos.y;
    return ne ? (nw ? ExitDirEnum::NORTH : ExitDirEnum::EAST)
              : (nw ? ExitDirEnum::WEST : ExitDirEnum::SOUTH);
}

void ConnectionSelection::setFirst(const RoomId id, const ExitDirEnum dir)
{
    m_first = true;
    m_connectionDescriptor[0].room.reset();
    if (const auto &room = m_map.findRoomHandle(id)) {
        this->receiveRoom(deref(room));
    }
    m_connectionDescriptor[0].direction = dir;
}

void ConnectionSelection::setSecond(const MouseSel &sel)
{
    m_first = false;
    const Coordinate c = sel.getCoordinate();
    m_connectionDescriptor[1].room.reset();
    if (const auto &room = m_map.findRoomHandle(c)) {
        this->receiveRoom(deref(room));
    }
    m_connectionDescriptor[1].direction = computeDirection(sel.pos);
}

void ConnectionSelection::setSecond(const RoomId id, const ExitDirEnum dir)
{
    m_first = false;
    m_connectionDescriptor[1].room.reset();
    if (const auto &room = m_map.findRoomHandle(id)) {
        this->receiveRoom(deref(room));
    }
    m_connectionDescriptor[1].direction = dir;
}

void ConnectionSelection::removeSecond()
{
    m_connectionDescriptor[1].room.reset();
}

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getFirst() const
{
    return m_connectionDescriptor[0];
}

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getSecond() const
{
    return m_connectionDescriptor[1];
}

void ConnectionSelection::receiveRoom(const RoomHandle &room)
{
    m_connectionDescriptor[m_first ? 0 : 1].room = room;
}

bool ConnectionSelection::ConnectionDescriptor::isTwoWay(const ConnectionDescriptor &first,
                                                         const ConnectionDescriptor &second)
{
    const auto &r1 = deref(first.room);
    const auto &r2 = deref(second.room);
    const auto &exit1 = r1.getExit(first.direction);
    const auto &exit2 = r2.getExit(second.direction);
    const RoomId &id1 = r1.getId();
    const RoomId &id2 = r2.getId();
    return exit1.containsOut(id2) && exit2.containsOut(id1);
}

bool ConnectionSelection::ConnectionDescriptor::isOneWay(const ConnectionDescriptor &first,
                                                         const ConnectionDescriptor &second)
{
    const auto &r1 = deref(first.room);
    const auto &r2 = deref(second.room);
    const ExitDirEnum dir2 = second.direction;
    const auto &exit1 = r1.getExit(first.direction);
    const auto &exit2 = r2.getExit(dir2);
    const RoomId &id1 = r1.getId();
    const RoomId &id2 = r2.getId();
    return exit1.containsOut(id2) && dir2 == ExitDirEnum::UNKNOWN && !exit2.containsOut(id1)
           && !exit1.containsIn(id2);
}

bool ConnectionSelection::ConnectionDescriptor::isCompleteExisting(
    const ConnectionDescriptor &first, const ConnectionDescriptor &second)
{
    return isTwoWay(first, second) || isOneWay(first, second);
}

bool ConnectionSelection::ConnectionDescriptor::isCompleteNew(
    const ConnectionSelection::ConnectionDescriptor &first,
    const ConnectionSelection::ConnectionDescriptor &second)
{
    return !isTwoWay(first, second);
}
