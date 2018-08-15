/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include "connectionselection.h"

#include <cassert>
#include <utility>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/room.h"
#include "../expandoracommon/roomadmin.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapfrontend.h"

// REVISIT: Duplication between here and mapcanvas.cpp
int ConnectionSelection::GLtoMap(const float arg)
{
    if (arg >= 0) {
        return static_cast<int>(arg + 0.5f);
    }
    return static_cast<int>(arg - 0.5f);
}

ConnectionSelection::ConnectionSelection()
{
    for (auto &x : m_connectionDescriptor)
        assert(x.room == nullptr);
}

ConnectionSelection::ConnectionSelection(MapFrontend *const mf,
                                         const float mx,
                                         const float my,
                                         const int layer)
{
    for (auto &x : m_connectionDescriptor)
        assert(x.room == nullptr);

    const Coordinate c(GLtoMap(mx), GLtoMap(my), layer);
    mf->lookingForRooms(*this, c, c);
    m_connectionDescriptor[0].direction = ComputeDirection(mx, my);
}

ConnectionSelection::~ConnectionSelection()
{
    //for all rooms remove them internaly and call release room
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

ExitDirection ConnectionSelection::ComputeDirection(const float mouseX, const float mouseY)
{
    ExitDirection dir = ExitDirection::UNKNOWN;

    //room centre
    //int x1 = (int) (mouseX + 0.5);
    //int y1 = (int) (mouseY + 0.5);
    const int x1 = GLtoMap(mouseX);
    const int y1 = GLtoMap(mouseY);

    float x1d = mouseX - static_cast<float>(x1);
    float y1d = mouseY - static_cast<float>(y1);

    if (y1d > -0.2f && y1d < 0.2f) {
        //y1p = y1;
        if (x1d >= 0.2f) {
            dir = ExitDirection::EAST;
            //x1p = x1 + 0.4;
        } else {
            if (x1d <= -0.2f) {
                dir = ExitDirection::WEST;
                //x1p = x1 - 0.4;
            } else {
                dir = ExitDirection::UNKNOWN;
                //x1p = x1;
            }
        }
    } else {
        //x1p = x1;
        if (y1d >= 0.2f) {
            //y1p = y1 + 0.4;
            dir = ExitDirection::SOUTH;
            if (x1d <= -0.2f) {
                dir = ExitDirection::DOWN;
                //x1p = x1 + 0.4;
            }
        } else {
            if (y1d <= -0.2f) {
                //y1p = y1 - 0.4;
                dir = ExitDirection::NORTH;
                if (x1d >= 0.2f) {
                    dir = ExitDirection::UP;
                    //x1p = x1 - 0.4;
                }
            }
        }
    }

    return dir;
}

/* TODO: refactor xxx{First,Second} to call the same functions with different argument */
void ConnectionSelection::setFirst(MapFrontend *const mf,
                                   const float mx,
                                   const float my,
                                   const int layer)
{
    m_first = true;
    const Coordinate c(GLtoMap(mx), GLtoMap(my), layer);
    if (m_connectionDescriptor[0].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[0].room->getId());
        }
        m_connectionDescriptor[0].room = nullptr;
    }
    mf->lookingForRooms(*this, c, c);
    m_connectionDescriptor[0].direction = ComputeDirection(mx, my);
    //if (m_connectionDescriptor[0].direction == CD_NONE) m_connectionDescriptor[0].direction = CD_UNKNOWN;
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
    //if (m_connectionDescriptor[0].direction == CD_NONE) m_connectionDescriptor[0].direction = CD_UNKNOWN;
}

void ConnectionSelection::setSecond(MapFrontend *const mf,
                                    const float mx,
                                    const float my,
                                    const int layer)
{
    m_first = false;
    Coordinate c(GLtoMap(mx), GLtoMap(my), layer);
    if (m_connectionDescriptor[1].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[1].room->getId());
        }
        m_connectionDescriptor[1].room = nullptr;
    }
    mf->lookingForRooms(*this, c, c);
    m_connectionDescriptor[1].direction = ComputeDirection(mx, my);
    //if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;
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
    //if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;
}

void ConnectionSelection::removeFirst()
{
    if (m_connectionDescriptor[0].room != nullptr) {
        if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room) {
            m_admin->releaseRoom(*this, m_connectionDescriptor[0].room->getId());
        }
        m_connectionDescriptor[0].room = nullptr;
    }
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
    //addroom to internal map
    //quint32 id = aRoom->getId();

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
