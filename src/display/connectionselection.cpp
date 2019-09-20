// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "connectionselection.h"

#include <cassert>
#include <utility>

#include "../configuration/NamedConfig.h"
#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapfrontend.h"

ConnectionSelection::ConnectionSelection(this_is_private)
{
    for (auto &x : m_connectionDescriptor)
        assert(x.room == nullptr);
}

ConnectionSelection::ConnectionSelection(this_is_private, MapFrontend *mf, const MouseSel &sel)
{
    for (auto &x : m_connectionDescriptor)
        assert(x.room == nullptr);

    const Coordinate c = sel.getCoordinate();
    mf->lookingForRooms(*this, c, c);
    m_connectionDescriptor[0].direction = computeDirection(sel.pos);
}

ConnectionSelection::~ConnectionSelection()
{
    // for all rooms remove them internaly and call release room
    if (auto *const admin = std::exchange(m_admin, nullptr)) {
        for (const auto &x : m_connectionDescriptor) {
            if (auto *const room = x.room) {
                admin->releaseRoom(*this, room->getId());
            }
        }
    }
}

bool ConnectionSelection::isValid() const
{
    for (const auto &x : m_connectionDescriptor)
        if (x.room == nullptr)
            return false;
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

    if (glm::distance(pos, upCenter) <= upDownRadius)
        return ExitDirEnum::UP;
    else if (glm::distance(pos, downCenter) <= upDownRadius)
        return ExitDirEnum::DOWN;
    else if (glm::distance(pos, actualCenter) <= centerRadius)
        return ExitDirEnum::UNKNOWN;

    const bool ne = pos.x >= 1.f - pos.y;
    const bool nw = pos.x <= pos.y;
    return ne ? (nw ? ExitDirEnum::NORTH : ExitDirEnum::EAST)
              : (nw ? ExitDirEnum::WEST : ExitDirEnum::SOUTH);
}

void ConnectionSelection::setFirst(MapFrontend *const mf, const RoomId id, const ExitDirEnum dir)
{
    m_first = true;
    if (m_connectionDescriptor[0].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[0].room->getId());
        }
        m_connectionDescriptor[0].room = nullptr;
    }
    mf->lookingForRooms(*this, id);
    m_connectionDescriptor[0].direction = dir;
    // if (m_connectionDescriptor[0].direction == CD_NONE) m_connectionDescriptor[0].direction = CD_UNKNOWN;
}

void ConnectionSelection::setSecond(MapFrontend *const mf, const MouseSel &sel)
{
    m_first = false;
    const Coordinate c = sel.getCoordinate();
    if (m_connectionDescriptor[1].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[1].room->getId());
        }
        m_connectionDescriptor[1].room = nullptr;
    }
    mf->lookingForRooms(*this, c, c);
    m_connectionDescriptor[1].direction = computeDirection(sel.pos);
    // if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;
}

void ConnectionSelection::setSecond(MapFrontend *const mf, const RoomId id, const ExitDirEnum dir)
{
    m_first = false;
    if (m_connectionDescriptor[1].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[1].room->getId());
        }
        m_connectionDescriptor[1].room = nullptr;
    }
    mf->lookingForRooms(*this, id);
    m_connectionDescriptor[1].direction = dir;
    // if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;
}

void ConnectionSelection::removeSecond()
{
    if (m_connectionDescriptor[1].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[1].room->getId());
        }
        m_connectionDescriptor[1].room = nullptr;
    }
}

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getFirst() const
{
    return m_connectionDescriptor[0];
}

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getSecond() const
{
    return m_connectionDescriptor[1];
}

void ConnectionSelection::receiveRoom(RoomAdmin *const admin, const Room *const aRoom)
{
    m_admin = admin;
    // addroom to internal map
    // quint32 id = aRoom->getId();

    if (m_first) {
        if (m_connectionDescriptor[0].room != nullptr) {
            if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
                m_admin->releaseRoom(*this, m_connectionDescriptor[0].room->getId());
            }
            m_connectionDescriptor[0].room = aRoom;
        } else {
            m_connectionDescriptor[0].room = aRoom;
        }
    } else {
        if (m_connectionDescriptor[1].room != nullptr) {
            if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
                m_admin->releaseRoom(*this, m_connectionDescriptor[1].room->getId());
            }
            m_connectionDescriptor[1].room = aRoom;
        } else {
            m_connectionDescriptor[1].room = aRoom;
        }
    }
}

bool ConnectionSelection::ConnectionDescriptor::isTwoWay(const ConnectionDescriptor &first,
                                                         const ConnectionDescriptor &second)
{
    const Room &r1 = deref(first.room);
    const Room &r2 = deref(second.room);
    const Exit &exit1 = r1.exit(first.direction);
    const Exit &exit2 = r2.exit(second.direction);
    const RoomId &id1 = r1.getId();
    const RoomId &id2 = r2.getId();
    return exit1.containsOut(id2) && exit2.containsOut(id1);
}

bool ConnectionSelection::ConnectionDescriptor::isOneWay(const ConnectionDescriptor &first,
                                                         const ConnectionDescriptor &second)
{
    const Room &r1 = deref(first.room);
    const Room &r2 = deref(second.room);
    const ExitDirEnum dir2 = second.direction;
    const Exit &exit1 = r1.exit(first.direction);
    const Exit &exit2 = r2.exit(dir2);
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
