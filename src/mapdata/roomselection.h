#pragma once
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

#ifndef ROOMSELECTION_H
#define ROOMSELECTION_H

#include <QMap>

#include "../expandoracommon/RoomRecipient.h"
#include "../global/roomid.h"

class Room;
class RoomAdmin;
class RoomSelection;
class MapData;

using SharedRoomSelection = std::shared_ptr<RoomSelection>;

/* Handle used for QT signals */
class SigRoomSelection final // was using SigRoomSelection = SharedRoomSelection;
{
private:
    SharedRoomSelection m_sharedRoomSelection{};

public:
    explicit SigRoomSelection(std::nullptr_t) {}
    explicit SigRoomSelection(const SharedRoomSelection &event)
        /* throws invalid argument */
        noexcept(false);

public:
    explicit SigRoomSelection() = default; /* required by QT */
    SigRoomSelection(SigRoomSelection &&) = default;
    SigRoomSelection(const SigRoomSelection &) = default;
    SigRoomSelection &operator=(SigRoomSelection &&) = default;
    SigRoomSelection &operator=(const SigRoomSelection &) = default;
    ~SigRoomSelection() = default;

public:
    // keep as non-inline for debugging
    bool isValid() const { return m_sharedRoomSelection != nullptr; }
    inline explicit operator bool() const { return isValid(); }

public:
    inline bool operator==(std::nullptr_t) const { return m_sharedRoomSelection == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return m_sharedRoomSelection != nullptr; }

public:
    inline bool operator==(const SigRoomSelection &rhs) const
    {
        return m_sharedRoomSelection == rhs.m_sharedRoomSelection;
    }
    inline bool operator!=(const SigRoomSelection &rhs) const { return !(*this == rhs); }

public:
    inline const SigRoomSelection &requireValid() const
        /* throws invalid argument */
        noexcept(false)
    {
        if (!isValid())
            throw std::runtime_error("invalid argument");
        return *this;
    }

public:
    const SharedRoomSelection &getShared() const
        /* throws null pointer */
        noexcept(false);

public:
    RoomSelection &deref() const
        /* throws null pointer */
        noexcept(false);
};

class RoomSelection : public QMap<RoomId, const Room *>, public RoomRecipient
{
public:
    explicit RoomSelection(MapData *admin);
    ~RoomSelection() override;
    void receiveRoom(RoomAdmin *admin, const Room *aRoom) override;

private:
    MapData *m_admin = nullptr;
};

#endif
