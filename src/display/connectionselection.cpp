// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "connectionselection.h"

#include <cassert>
#include <utility>

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapfrontend.h"

ConnectionSelection::ConnectionSelection()
{
    for (auto &x : m_connectionDescriptor)
        assert(x.room == nullptr);
}

ConnectionSelection::ConnectionSelection(MapFrontend *mf, const MouseSel &sel)
{
    for (auto &x : m_connectionDescriptor)
        assert(x.room == nullptr);

    const Coordinate c = sel.getCoordinate();
    mf->lookingForRooms(*this, c, c);
    m_connectionDescriptor[0].direction = ComputeDirection(sel.pos);
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

bool ConnectionSelection::isValid()
{
    for (const auto &x : m_connectionDescriptor)
        if (x.room == nullptr)
            return false;
    return true;
}

ExitDirection ConnectionSelection::ComputeDirection(const Coordinate2f &mouse_f)
{
    ExitDirection dir = ExitDirection::UNKNOWN;
    const auto mouse = mouse_f.round();

    const int x1 = mouse.x;
    const int y1 = mouse.y;

    const float x1d = mouse_f.x - static_cast<float>(x1);
    const float y1d = mouse_f.y - static_cast<float>(y1);

    if (y1d > -0.2f && y1d < 0.2f) {
        // y1p = y1;
        if (x1d >= 0.2f) {
            dir = ExitDirection::EAST;
            // x1p = x1 + 0.4;
        } else {
            if (x1d <= -0.2f) {
                dir = ExitDirection::WEST;
                // x1p = x1 - 0.4;
            } else {
                dir = ExitDirection::UNKNOWN;
                // x1p = x1;
            }
        }
    } else {
        // x1p = x1;
        if (y1d >= 0.2f) {
            // y1p = y1 + 0.4;
            dir = ExitDirection::SOUTH;
            if (x1d <= -0.2f) {
                dir = ExitDirection::DOWN;
                // x1p = x1 + 0.4;
            }
        } else {
            if (y1d <= -0.2f) {
                // y1p = y1 - 0.4;
                dir = ExitDirection::NORTH;
                if (x1d >= 0.2f) {
                    dir = ExitDirection::UP;
                    // x1p = x1 - 0.4;
                }
            }
        }
    }

    return dir;
}

void ConnectionSelection::setFirst(MapFrontend *const mf, const RoomId id, const ExitDirection dir)
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
    m_connectionDescriptor[1].direction = ComputeDirection(sel.pos);
    // if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;
}

void ConnectionSelection::setSecond(MapFrontend *const mf, const RoomId id, const ExitDirection dir)
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

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getFirst()
{
    return m_connectionDescriptor[0];
}

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getSecond()
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
