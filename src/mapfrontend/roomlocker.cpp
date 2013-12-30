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

#include "roomlocker.h"
#include "mapfrontend.h"
#include "abstractroomfactory.h"
#include "roomrecipient.h"

RoomLocker::RoomLocker(RoomRecipient * forward, MapFrontend * frontend, AbstractRoomFactory * in_factory, ParseEvent * compare) :
        recipient(forward),
        data(frontend),
        factory(in_factory),
comparator(compare) {}


RoomOutStream & RoomLocker::operator<<(const Room * room) {
    if (factory && comparator) {
        if (factory->compareWeakProps(room, comparator) != CR_DIFFERENT) {
            data->lockRoom(recipient, room->getId());
            recipient->receiveRoom(data, room);
        }
    } else {
        data->lockRoom(recipient, room->getId());
        recipient->receiveRoom(data, room);
    }
    return *this;
}
