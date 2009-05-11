/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara),
**            Anonymous <lachupe@gmail.com> (Azazello)
**
** This file is part of the MMapper project. 
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
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

#ifndef ROOMADMIN_H
#define ROOMADMIN_H

#include "parseevent.h"

class RoomRecipient;
class MapAction;

class RoomAdmin {
  public:
    // removes the lock on a room
    // after the last lock is removed, the room is deleted
    virtual void releaseRoom(RoomRecipient *, uint) = 0;

    // makes a lock on a room permanent and anonymous.
    // Like that the room can't be deleted via releaseRoom anymore.
    virtual void keepRoom(RoomRecipient *, uint) = 0;
    
    virtual void scheduleAction(MapAction * action) = 0;
    
    virtual ~RoomAdmin() {};
};

#endif
