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

#include "roompropertysetter.h"
#include "customaction.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "room.h"
#include "roomadmin.h"

void RoomPropertySetterSlave::receiveRoom(RoomAdmin *admin, const Room *room)
{
    admin->scheduleAction(new SingleRoomAction(action, room->getId()));
    action = nullptr;
}

RoomPropertySetter::RoomPropertySetter() : Component(false)
{
    propPositions["name"] = static_cast<uint>(R_NAME);
    propPositions["desc"] = static_cast<uint>(R_DESC);
    propPositions["terrain"] = static_cast<uint>(R_TERRAINTYPE);
    propPositions["dynamicDesc"] = static_cast<uint>(R_DYNAMICDESC);
    propPositions["note"] = static_cast<uint>(R_NOTE);
    // R_MOBFLAGS, R_LOADFLAGS,
    propPositions["portable"] = static_cast<uint>(R_PORTABLETYPE);
    propPositions["light"] = static_cast<uint>(R_LIGHTTYPE);
    propPositions["align"] = static_cast<uint>(R_ALIGNTYPE);
    propPositions["ridable"] = static_cast<uint>(R_RIDABLETYPE);
    /*QMap<QByteArray, uint> roomMobFlags;
    QMap<QByteArray, uint> roomLoadFlags;
    QMap<QByteArray, uint> exitProps;
    QMap<QByteArray, uint> exitFlags;
    QMap<QByteArray, uint> enumValues;*/
}

void RoomPropertySetter::parseProperty(const QByteArray &command, const Coordinate &roomPos)
{
    QList<QByteArray> words = command.simplified().split(' ');
    AbstractAction *action = nullptr;
    QByteArray property = words[1];
    uint pos = propPositions[property];
    if (words.size() == 4) {
        //change exit property
        ExitDirection dir = Mmapper2Exit::dirForChar(words[2][0]);
        switch (pos) {
        case E_FLAGS:
        case E_DOORFLAGS:
            action = new ModifyExitFlags(fieldValues[property], dir, pos, FMM_TOGGLE);
            break;
        case E_DOORNAME:
            action = new UpdateExitField(property, dir, pos);
            break;
        default:
            emit sendToUser("unknown property: " + property + "\r\n");
            return;
        }
    } else if (words.size() == 3) {
        //change room property
        switch (pos) {
        case R_TERRAINTYPE:
            action = new UpdatePartial(fieldValues[property], pos);
            break;
        case R_NAME:
        case R_DESC:
            action = new UpdatePartial(property, pos);
            break;
        case R_MOBFLAGS:
        case R_LOADFLAGS:
            action = new ModifyRoomFlags(fieldValues[property], pos, FMM_TOGGLE);
            break;
        case R_DYNAMICDESC:
        case R_NOTE:
            action = new UpdateRoomField(property, pos);
            break;
        case R_PORTABLETYPE:
        case R_LIGHTTYPE:
        case R_ALIGNTYPE:
        case R_RIDABLETYPE:
            action = new UpdateRoomField(fieldValues[property], pos);
            break;
        default:
            emit sendToUser("unknown property: " + property + "\r\n");
            return;
        }
    }

    if (action != nullptr) {
        RoomPropertySetterSlave slave(action);
        emit lookingForRooms(&slave, roomPos);

        if (slave.getResult()) {
            emit sendToUser("OK\r\n");
        } else {
            emit sendToUser("setting " + property + " failed!\r\n");
        }
    }
}
