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

#include "roomfactory.h"
#include "coordinate.h"
#include "mmapper2event.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "room.h"

#include <QStringList>

const QRegExp RoomFactory::whitespace = QRegExp("\\s+");

Room *RoomFactory::createRoom(const ParseEvent *ev) const
{
    auto *room = new Room(NUM_ROOM_PROPS, NUM_EXITS, NUM_EXIT_PROPS);
    if (ev != nullptr) {
        update(room, ev);
    }
    return room;
}


ParseEvent *RoomFactory::getEvent(const Room *room) const
{
    ExitsFlagsType exitFlags = 0;
    for (uint dir = 0; dir < 6; ++dir) {
        const Exit &e = room->exit(dir);

        ExitFlags eFlags = Mmapper2Exit::getFlags(e);
        exitFlags |= (eFlags << (dir * 3));
    }

    exitFlags |= EXITS_FLAGS_VALID;
    PromptFlagsType promptFlags = Mmapper2Room::getTerrainType(room);
    promptFlags |= PROMPT_FLAGS_VALID;

    CommandIdType c = CID_UNKNOWN;

    ParseEvent *event = Mmapper2Event::createEvent(c, Mmapper2Room::getName(room),
                                                   Mmapper2Room::getDynamicDescription(room),
                                                   Mmapper2Room::getDescription(room), exitFlags, promptFlags, 0);
    return event;
}

ComparisonResult RoomFactory::compareStrings(const QString &room, const QString &event,
                                             uint prevTolerance, bool updated) const
{
    prevTolerance *= room.size();
    prevTolerance /= 100;
    int tolerance = prevTolerance;


    QStringList descWords = room.split(whitespace, QString::SkipEmptyParts);
    QStringList eventWords = event.split(whitespace, QString::SkipEmptyParts);

    if (!eventWords.isEmpty()) { // if event is empty we don't compare
        while (tolerance >= 0) {
            if (descWords.isEmpty()) {
                if (updated) { // if notUpdated the desc is allowed to be shorter than the event
                    tolerance -= eventWords.join(" ").size();
                }
                break;
            }
            if (eventWords.isEmpty()) { // if we get here the event isn't empty
                tolerance -= descWords.join(" ").size();
                break;
            }

            QString eventWord = eventWords.takeFirst();
            QString descWord = descWords.takeFirst();
            int descSize = descWord.size();
            int eventSize = eventWord.size();
            if (descSize > eventSize) {
                tolerance -= (descSize - eventSize);
            } else {
                tolerance -= (eventSize - descSize);
                eventSize = descSize;
            }
            for (; eventSize >= 0; --eventSize) {
                if (eventWord[eventSize] != descWord[eventSize]) {
                    --tolerance;
                }
            }
        }
    }

    if (tolerance < 0) {
        return CR_DIFFERENT;
    } else if (static_cast<int>(prevTolerance) != tolerance) {
        return CR_TOLERANCE;
    } else if (event.size() != room.size()) { // differences in amount of whitespace
        return CR_TOLERANCE;
    }
    return CR_EQUAL;


}

ComparisonResult RoomFactory::compare(const Room *room, const ParseEvent *event,
                                      uint tolerance) const
{
    QString name = Mmapper2Room::getName(room);
    QString m_desc = Mmapper2Room::getDescription(room);
    RoomTerrainType terrainType = Mmapper2Room::getTerrainType(room);
    bool updated = room->isUpToDate();

    if (event == nullptr) {
        return CR_EQUAL;
    }

    if (tolerance >= 100 || (name.isEmpty() && m_desc.isEmpty() && (!updated))) {
        // user-created oder explicit update
        return CR_TOLERANCE;
    }

    PromptFlagsType pf = Mmapper2Event::getPromptFlags(event);
    if ((pf & PROMPT_FLAGS_VALID) != 0) {
        if ((pf & TERRAIN_TYPE) != terrainType) {
            if (room->isUpToDate()) {
                return CR_DIFFERENT;
            }
        }
    }

    switch (compareStrings(name, Mmapper2Event::getRoomName(event), tolerance)) {
    case CR_TOLERANCE:
        updated = false;
        break;
    case CR_DIFFERENT:
        return CR_DIFFERENT;
    case CR_EQUAL:
        break;
    }

    switch (compareStrings(m_desc, Mmapper2Event::getParsedRoomDesc(event), tolerance, updated)) {
    case CR_TOLERANCE:
        updated = false;
        break;
    case CR_DIFFERENT:
        return CR_DIFFERENT;
    case CR_EQUAL:
        break;
    }

    switch (compareWeakProps(room, event)) {
    case CR_DIFFERENT:
        return CR_DIFFERENT;
    case CR_TOLERANCE:
        updated = false;
        break;
    case CR_EQUAL:
        break;
    }

    if (updated) {
        return CR_EQUAL;
    }
    return CR_TOLERANCE;
}

ComparisonResult RoomFactory::compareWeakProps(const Room *room, const ParseEvent *event,
                                               uint /*tolerance*/) const
{
    bool exitsValid = room->isUpToDate();
    bool different = false;
    bool tolerance = false;

    PromptFlagsType pFlags = Mmapper2Event::getPromptFlags(event);
    if ((pFlags & PROMPT_FLAGS_VALID) != 0) {
        const RoomLightType lightType = Mmapper2Room::getLightType(room);
        const RoomSundeathType sunType = Mmapper2Room::getSundeathType(room);
        if (((pFlags & LIT_ROOM) != 0) && lightType != RLT_LIT && sunType == RST_NOSUNDEATH) {
            // Allow prompt sunlight to override rooms with the DARK flag if we know the room
            // is troll safe and obviously not in permanent darkness
            tolerance = true;
        }
        /*
        // Disable until we can identify day/night or blindness/darkness spell
        else if ((pFlags & DARK_ROOM) && lightType != RLT_DARK) {
            tolerance = true;
        }
        */
    }

    ExitsFlagsType eFlags = Mmapper2Event::getExitFlags(event);
    if ((eFlags & EXITS_FLAGS_VALID) != 0u) {
        for (uint dir = 0; dir < 6; ++dir) {
            const Exit &e = room->exit(dir);
            ExitFlags mFlags = Mmapper2Exit::getFlags(e);
            if (mFlags != 0u) {
                // exits are considered valid as soon as one exit is found (or if the room is updated
                exitsValid = true;
                if (different) {
                    return CR_DIFFERENT;
                }
            }
            if ((mFlags & EF_NO_MATCH) != 0) {
                continue;
            }
            ExitsFlagsType eThisExit = eFlags >> (dir * 4);
            ExitsFlagsType diff = (eThisExit ^ mFlags);
            if ((diff & (EF_EXIT | EF_DOOR)) != 0u) {
                if (exitsValid) {
                    if (!tolerance) {
                        if (((mFlags & EF_EXIT) == 0)
                                && ((eThisExit & EF_DOOR) != 0u)) { // We have no exit on record and there is a secret door
                            tolerance = true;
                        } else if ((mFlags & EF_DOOR) != 0) { // We have a secret door on record
                            tolerance = true;
                        } else {
                            return CR_DIFFERENT;
                        }
                    } else {
                        return CR_DIFFERENT;
                    }
                } else {
                    different = true;
                }
            } else if ((diff & EF_ROAD) != 0u) {
                tolerance = true;
            } else if ((diff & EF_CLIMB) != 0u) {
                tolerance = true;
            }

        }
    }
    if (tolerance || !exitsValid) {
        return CR_TOLERANCE;
    }
    return CR_EQUAL;
}

void RoomFactory::update(Room *room, const ParseEvent *event) const
{
    (*room)[R_DYNAMICDESC] = Mmapper2Event::getRoomDesc(event);

    ExitsFlagsType eFlags = Mmapper2Event::getExitFlags(event);
    if ((eFlags & EXITS_FLAGS_VALID) != 0u) {
        eFlags ^= EXITS_FLAGS_VALID;
        if (!room->isUpToDate()) {
            for (int dir = 0; dir < 6; ++dir) {

                ExitFlags mFlags = (eFlags >> (dir * 4) & (EF_EXIT | EF_DOOR | EF_ROAD | EF_CLIMB));

                Exit &e = room->exit(dir);
                if (((Mmapper2Exit::getFlags(e) & EF_DOOR) != 0)
                        && ((mFlags & EF_DOOR) == 0)) { // We have a secret door on record
                    mFlags |= EF_DOOR;
                    mFlags |= EF_EXIT;
                }
                if (((mFlags & EF_DOOR) != 0)
                        && ((mFlags & EF_ROAD) != 0)) { // TODO(nschimme): Door and road is confusing??
                    mFlags |= EF_NO_MATCH;
                }
                e[E_FLAGS] = mFlags;
            }
        } else {
            for (int dir = 0; dir < 6; ++dir) {
                Exit &e = room->exit(dir);
                ExitFlags mFlags = (eFlags >> (dir * 4) & (EF_EXIT | EF_DOOR | EF_ROAD | EF_CLIMB));
                Mmapper2Exit::updateExit(e, mFlags);
            }
        }
        room->setUpToDate();
    } else {
        room->setOutDated();
    }

    PromptFlagsType pFlags = Mmapper2Event::getPromptFlags(event);
    if ((pFlags & PROMPT_FLAGS_VALID) != 0) {
        PromptFlagsType rt = pFlags;
        rt &= TERRAIN_TYPE;
        (*room)[R_TERRAINTYPE] = static_cast<RoomTerrainType>(rt);
        if ((pFlags & LIT_ROOM) != 0) {
            (*room)[R_LIGHTTYPE] = RLT_LIT;
        }/* else if (pFlags & DARK_ROOM) {
            (*room)[R_LIGHTTYPE] = RLT_DARK;
        }*/
    } else {
        room->setOutDated();
    }

    QString desc = Mmapper2Event::getParsedRoomDesc(event);
    if (!desc.isEmpty()) {
        room->replace(R_DESC, desc);
    } else {
        room->setOutDated();
    }

    QString name = Mmapper2Event::getRoomName(event);
    if (!name.isEmpty()) {
        room->replace(R_NAME, name);
    } else {
        room->setOutDated();
    }
}


void RoomFactory::update(Room *target, const Room *source) const
{
    QString name = Mmapper2Room::getName(source);
    if (!name.isEmpty()) {
        target->replace(R_NAME, name);
    }
    QString desc = Mmapper2Room::getDescription(source);
    if (!desc.isEmpty()) {
        target->replace(R_DESC, desc);
    }
    QString dynamic = Mmapper2Room::getDynamicDescription(source);
    if (!dynamic.isEmpty()) {
        target->replace(R_DYNAMICDESC, dynamic);
    }

    if (Mmapper2Room::getAlignType(target) == RAT_UNDEFINED) {
        target->replace(R_ALIGNTYPE, Mmapper2Room::getAlignType(source));
    }
    if (Mmapper2Room::getLightType(target) == RLT_UNDEFINED) {
        target->replace(R_LIGHTTYPE, Mmapper2Room::getLightType(source));
    }
    if (Mmapper2Room::getSundeathType(target) == RST_UNDEFINED) {
        target->replace(R_SUNDEATHTYPE, Mmapper2Room::getSundeathType(source));
    }
    if (Mmapper2Room::getPortableType(target) == RPT_UNDEFINED) {
        target->replace(R_PORTABLETYPE, Mmapper2Room::getPortableType(source));
    }
    if (Mmapper2Room::getRidableType(target) == RRT_UNDEFINED) {
        target->replace(R_RIDABLETYPE, Mmapper2Room::getRidableType(source));
    }
    if (Mmapper2Room::getTerrainType(source) != RTT_UNDEFINED) {
        target->replace(R_TERRAINTYPE, Mmapper2Room::getTerrainType(source));
    }

    target->replace(R_NOTE, Mmapper2Room::getNote(target).append(Mmapper2Room::getNote(source)));

    target->replace(R_MOBFLAGS, Mmapper2Room::getMobFlags(target) | Mmapper2Room::getMobFlags(source));
    target->replace(R_LOADFLAGS, Mmapper2Room::getLoadFlags(target) | Mmapper2Room::getLoadFlags(
                        source));

    if (!target->isUpToDate()) {
        for (int dir = 0; dir < 6; ++dir) {
            const Exit &o = source->exit(dir);
            Exit &e = target->exit(dir);
            ExitFlags mFlags = Mmapper2Exit::getFlags(o);
            if (((Mmapper2Exit::getFlags(e) & EF_DOOR) != 0) && ((mFlags & EF_DOOR) == 0)) {
                mFlags |= EF_NO_MATCH;
                mFlags |= EF_DOOR;
                mFlags |= EF_EXIT;
            } else {
                e[E_DOORNAME] = o[E_DOORNAME];
                e[E_DOORFLAGS] = o[E_DOORFLAGS];
            }
            if (((mFlags & EF_DOOR) != 0) && ((mFlags & EF_ROAD) != 0)) {
                mFlags |= EF_NO_MATCH;
            }
            e[E_FLAGS] = mFlags;
        }
    } else {
        for (int dir = 0; dir < 6; ++dir) {
            Exit &e = target->exit(dir);
            const Exit &o = source->exit(dir);
            ExitFlags mFlags = Mmapper2Exit::getFlags(o);
            ExitFlags eFlags = Mmapper2Exit::getFlags(e);
            if (eFlags != mFlags) {
                e[E_FLAGS] = (eFlags | mFlags) | EF_NO_MATCH;
            }
            const QString &door = Mmapper2Exit::getDoorName(o);
            if (!door.isEmpty()) {
                e[E_DOORNAME] = door;
            }
            DoorFlags doorFlags = Mmapper2Exit::getDoorFlags(o);
            doorFlags |= Mmapper2Exit::getDoorFlags(e);
            e[E_DOORFLAGS] = doorFlags;
        }
    }
    if (source->isUpToDate()) {
        target->setUpToDate();
    }
}


uint RoomFactory::opposite(uint in) const
{
    switch (in) {
    case ED_NORTH:
        return ED_SOUTH;
    case ED_SOUTH:
        return ED_NORTH;
    case ED_WEST:
        return ED_EAST;
    case ED_EAST:
        return ED_WEST;
    case ED_UP:
        return ED_DOWN;
    case ED_DOWN:
        return ED_UP;
    default:
        return ED_UNKNOWN;
    }
}


const Coordinate &RoomFactory::exitDir(uint dir) const
{
    return exitDirs[dir];
}

RoomFactory::RoomFactory() : exitDirs(64)
{
    Coordinate north(0, -1, 0);
    exitDirs[CID_NORTH] = north;
    Coordinate south(0, 1, 0);
    exitDirs[CID_SOUTH] = south;
    Coordinate west(-1, 0, 0);
    exitDirs[CID_WEST] = west;
    Coordinate east(1, 0, 0);
    exitDirs[CID_EAST] = east;
    Coordinate up(0, 0, 1);
    exitDirs[CID_UP] = up;
    Coordinate down(0, 0, -1);
    exitDirs[CID_DOWN] = down;
    // rest is default constructed which means (0,0,0)
}


