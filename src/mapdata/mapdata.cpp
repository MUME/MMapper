/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "mapdata.h"
#include "customaction.h"
#include "drawstream.h"
#include "infomark.h"
#include "mmapper2room.h"
#include "roomfactory.h"
#include "roomselection.h"

#include <cassert>

MapData::MapData()
    : MapFrontend(new RoomFactory)
    , m_dataChanged(false)
{}

QString MapData::getDoorName(const Coordinate &pos, uint dir)
{
    QMutexLocker locker(&mapLock);
    Room *room = map.get(pos);
    if ((room != nullptr) && dir < 7) {
        return Mmapper2Exit::getDoorName(room->exit(dir));
    }
    return "exit";
}

void MapData::setDoorName(const Coordinate &pos, const QString &name, uint dir)
{
    QMutexLocker locker(&mapLock);
    Room *room = map.get(pos);
    if ((room != nullptr) && dir < 7) {
        /*
        // Is the Door there? If not, add it.
        if (!getExitFlag(pos, dir, EF_DOOR)) {
          MapAction * action_setdoor = new SingleRoomAction(new ModifyExitFlags(EF_DOOR, dir, E_FLAGS, FMM_SET), room->getId());
          scheduleAction(action_setdoor);
        }
        */
        setDataChanged();
        MapAction *action = new SingleRoomAction(new UpdateExitField(name, dir, E_DOORNAME),
                                                 room->getId());
        scheduleAction(action);
    }
}

bool MapData::getExitFlag(const Coordinate &pos, uint flag, uint dir, uint field)
{
    QMutexLocker locker(&mapLock);
    Room *room = map.get(pos);
    if ((room != nullptr) && dir < 7) {
        //ExitFlags ef = ::getFlags(room->exit(dir));
        ExitFlags ef = room->exit(dir)[field].toUInt();
        if (ISSET(ef, flag)) {
            return true;
        }
    }
    return false;
}

void MapData::toggleExitFlag(const Coordinate &pos, uint flag, uint dir, uint field)
{
    QMutexLocker locker(&mapLock);
    Room *room = map.get(pos);
    if ((room != nullptr) && dir < 7) {
        setDataChanged();
        MapAction *action = new SingleRoomAction(new ModifyExitFlags(flag, dir, field, FMM_TOGGLE),
                                                 room->getId());
        scheduleAction(action);
    }
}

void MapData::toggleRoomFlag(const Coordinate &pos, uint flag, uint field)
{
    QMutexLocker locker(&mapLock);
    Room *room = map.get(pos);
    if ((room != nullptr) && field < ROOMFIELD_LAST) {
        setDataChanged();
        MapAction *action = new SingleRoomAction(new ModifyRoomFlags(flag, field, FMM_TOGGLE),
                                                 room->getId());
        scheduleAction(action);
    }
}

bool MapData::getRoomFlag(const Coordinate &pos, uint flag, uint field)
{
    const QVariant val = getRoomField(pos, field);
    return !val.isNull() && ISSET(val.toUInt(), flag);
}

void MapData::setRoomField(const Coordinate &pos, const QVariant &flag, uint field)
{
    QMutexLocker locker(&mapLock);
    Room *room = map.get(pos);
    if ((room != nullptr) && field < ROOMFIELD_LAST) {
        setDataChanged();
        MapAction *action = new SingleRoomAction(new UpdateRoomField(flag, field), room->getId());
        scheduleAction(action);
    }
}

QVariant MapData::getRoomField(const Coordinate &pos, uint field)
{
    QMutexLocker locker(&mapLock);
    /* REVISIT: is it wise to just ignore bad data here? */
    if (field < ROOMFIELD_LAST) {
        if (Room *room = map.get(pos)) {
            return room->at(static_cast<RoomField>(field));
        }
    }
    return QVariant();
}

QList<Coordinate> MapData::getPath(const QList<CommandIdType> &dirs)
{
    QMutexLocker locker(&mapLock);
    QList<Coordinate> ret;
    if (Room *room = map.get(m_position)) {
        QListIterator<CommandIdType> iter(dirs);
        while (iter.hasNext()) {
            uint dir = iter.next();
            if (dir > 5) {
                break;
            }
            Exit &e = room->exit(dir);
            if ((Mmapper2Exit::getFlags(e) & EF_EXIT) == 0) {
                // REVISIT: why does this continue but all of the others break?
                continue;
            }
            if (!e.outIsUnique()) {
                break;
            }
            room = roomIndex[e.outFirst()];
            if (room == nullptr) {
                break;
            }
            ret.append(room->getPosition());
        }
    }
    return ret;
}

const RoomSelection *MapData::select(const Coordinate &ulf, const Coordinate &lrb)
{
    QMutexLocker locker(&mapLock);
    auto *selection = new RoomSelection(this);
    selections[selection] = selection;
    lookingForRooms(selection, ulf, lrb);
    return selection;
}
// updates a selection created by the mapdata
const RoomSelection *MapData::select(const Coordinate &ulf,
                                     const Coordinate &lrb,
                                     const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *selection = selections[in];
    assert(selection);
    lookingForRooms(selection, ulf, lrb);
    return selection;
}
// creates and registers a selection with one room
const RoomSelection *MapData::select(const Coordinate &pos)
{
    QMutexLocker locker(&mapLock);
    auto *selection = new RoomSelection(this);
    selections[selection] = selection;
    lookingForRooms(selection, pos);
    return selection;
}
// creates and registers an empty selection
const RoomSelection *MapData::select()
{
    QMutexLocker locker(&mapLock);
    auto *selection = new RoomSelection(this);
    selections[selection] = selection;
    return selection;
}
// removes the selection from the internal structures and deletes it
void MapData::unselect(const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *selection = selections[in];
    assert(selection);
    selections.erase(in);
    QMap<uint, const Room *>::iterator i = selection->begin();
    while (i != selection->end()) {
        keepRoom(selection, (i++).key());
    }
    delete selection;
}

// the room will be inserted in the given selection. the selection must have been created by mapdata
const Room *MapData::getRoom(const Coordinate &pos, const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *selection = selections[in];
    assert(selection);
    Room *room = map.get(pos);
    if (room != nullptr) {
        uint id = room->getId();
        locks[id].insert(selection);
        selection->insert(id, room);
    }
    return room;
}

const Room *MapData::getRoom(uint id, const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *selection = selections[in];
    assert(selection);
    Room *room = roomIndex[id];
    if (room != nullptr) {
        // REVISIT: is roomID the same as id?
        uint roomId = room->getId();
        locks[roomId].insert(selection);
        selection->insert(roomId, room);
    }
    return room;
}

void MapData::draw(const Coordinate &ulf, const Coordinate &lrb, MapCanvas &screen)
{
    QMutexLocker locker(&mapLock);
    DrawStream drawer(screen, roomIndex, locks);
    map.getRooms(drawer, ulf, lrb);
}

bool MapData::isOccupied(const Coordinate &position)
{
    QMutexLocker locker(&mapLock);
    return map.get(position) != nullptr;
}

bool MapData::isMovable(const Coordinate &offset, const RoomSelection *selection)
{
    QMutexLocker locker(&mapLock);
    QMap<uint, const Room *>::const_iterator i = selection->begin();
    const Room *other = nullptr;
    while (i != selection->end()) {
        Coordinate target = (*(i++))->getPosition() + offset;
        other = map.get(target);
        if ((other != nullptr) && !selection->contains(other->getId())) {
            return false;
        }
    }
    return true;
}

void MapData::unselect(uint id, const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *selection = selections[in];
    assert(selection);
    keepRoom(selection, id);
    selection->remove(id);
}

// selects the rooms given in "other" for "into"
const RoomSelection *MapData::select(const RoomSelection *other, const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *into = selections[in];
    assert(into);
    QMapIterator<uint, const Room *> i(*other);
    while (i.hasNext()) {
        locks[i.key()].insert(into);
        into->insert(i.key(), i.value());
    }
    return into;
}

// removes the subset from the superset and unselects
void MapData::unselect(const RoomSelection *subset, const RoomSelection *in)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *superset = selections[in];
    assert(superset);
    QMapIterator<uint, const Room *> i(*subset);
    while (i.hasNext()) {
        keepRoom(superset, i.key());
        superset->remove(i.key());
    }
}

bool MapData::execute(MapAction *action)
{
    QMutexLocker locker(&mapLock);
    action->schedule(this);
    bool executable = isExecutable(action);
    if (executable) {
        executeAction(action);
    }
    return executable;
}

bool MapData::execute(MapAction *action, const RoomSelection *unlock)
{
    QMutexLocker locker(&mapLock);
    action->schedule(this);
    std::list<uint> selectedIds;

    RoomSelection *selection = selections[unlock];
    assert(selection);
    {
        QMap<uint, const Room *>::iterator i = selection->begin();
        while (i != selection->end()) {
            const Room *room = *(i++);
            uint id = room->getId();
            locks[id].erase(selection);
            selectedIds.push_back(id);
        }
    }
    selection->clear();

    bool executable = isExecutable(action);
    if (executable) {
        executeAction(action);
    }

    for (auto id : selectedIds) {
        Room *room = roomIndex[id];
        if (room != nullptr) {
            locks[id].insert(selection);
            selection->insert(id, room);
        }
    }
    delete action;
    return executable;
}

void MapData::clear()
{
    MapFrontend::clear();
    while (!m_markers.isEmpty()) {
        delete m_markers.takeFirst();
    }
    emit log("MapData", "cleared MapData");
}

void MapData::removeDoorNames()
{
    QMutexLocker locker(&mapLock);

    for (auto &room : roomIndex) {
        if (room != nullptr) {
            for (uint dir = 0; dir <= 6; dir++) {
                MapAction *action = new SingleRoomAction(new UpdateExitField("", dir, E_DOORNAME),
                                                         room->getId());
                scheduleAction(action);
            }
        }
        setDataChanged();
    }
}

void MapData::genericSearch(RoomRecipient *recipient, const RoomFilter &f)
{
    QMutexLocker locker(&mapLock);
    for (auto &room : roomIndex) {
        if (room != nullptr) {
            if (f.filter(room)) {
                locks[room->getId()].insert(recipient);
                recipient->receiveRoom(this, room);
            }
        }
    }
}

void MapData::genericSearch(const RoomSelection *in, const RoomFilter &f)
{
    QMutexLocker locker(&mapLock);
    RoomSelection *selection = selections[in];
    assert(selection);
    genericSearch(dynamic_cast<RoomRecipient *>(selection), f);
}

MapData::~MapData()
{
    while (!m_markers.isEmpty()) {
        delete m_markers.takeFirst();
    }
}
bool MapData::execute(AbstractAction *action, const RoomSelection *unlock)
{
    return execute(new GroupAction(action, unlock), unlock);
}
void MapData::removeMarker(InfoMark *im)
{
    m_markers.removeAll(im);
    delete im;
}
void MapData::addMarker(InfoMark *im)
{
    m_markers.append(im);
}
