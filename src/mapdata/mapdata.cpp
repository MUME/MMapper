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

#include <cassert>
#include <list>
#include <map>
#include <set>
#include <QList>
#include <QString>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapfrontend/map.h"
#include "../mapfrontend/mapaction.h"
#include "../mapfrontend/mapfrontend.h"
#include "../parser/CommandId.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "customaction.h"
#include "drawstream.h"
#include "infomark.h"
#include "mmapper2room.h"
#include "roomfactory.h"
#include "roomfilter.h"
#include "roomselection.h"

MapData::MapData(QObject *const parent)
    : MapFrontend(new RoomFactory{}, parent)
{}

QString MapData::getDoorName(const Coordinate &pos, const ExitDirection dir)
{
    // REVISIT: Could this function could be made const if we make mapLock mutable?
    // Alternately, WTF are we accessing this from multiple threads?
    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirection::UNKNOWN) {
            return room->exit(dir).getDoorName();
        }
    }
    return "exit";
}

void MapData::setDoorName(const Coordinate &pos, const QString &name, const ExitDirection dir)
{
    QMutexLocker locker(&mapLock);
    const auto doorName = DoorName{name};
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirection::UNKNOWN) {
            setDataChanged();
            MapAction *action = new SingleRoomAction(new UpdateExitField(doorName, dir),
                                                     room->getId());
            scheduleAction(action);
        }
    }
}

bool MapData::getExitFlag(const Coordinate &pos, const ExitDirection dir, ExitFieldVariant var)
{
    assert(var.getType() != ExitField::DOOR_NAME);

    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirection::NONE) {
            switch (var.getType()) {
            case ExitField::DOOR_NAME: {
                const auto name = room->exit(dir).getDoorName();
                if (var.getDoorName() == name) {
                    return true;
                }
                break;
            }
            case ExitField::EXIT_FLAGS: {
                const auto ef = room->exit(dir).getExitFlags();
                if (IS_SET(ef, var.getExitFlags())) {
                    return true;
                }
                break;
            }
            case ExitField::DOOR_FLAGS: {
                const auto df = room->exit(dir).getDoorFlags();
                if (IS_SET(df, var.getDoorFlags())) {
                    return true;
                }
                break;
            }
            }
        }
    }
    return false;
}

void MapData::toggleExitFlag(const Coordinate &pos, const ExitDirection dir, ExitFieldVariant var)
{
    const auto field = var.getType();
    assert(field != ExitField::DOOR_NAME);

    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirection::NONE) {
            setDataChanged();

            MapAction *action = new SingleRoomAction(new ModifyExitFlags(var,
                                                                         dir,
                                                                         FlagModifyMode::TOGGLE),
                                                     room->getId());
            scheduleAction(action);
        }
    }
}

void MapData::toggleRoomFlag(const Coordinate &pos, const uint flag, const RoomField field)
{
    QMutexLocker locker(&mapLock);
    if (Room *room = map.get(pos)) {
        if (field < RoomField::LAST) {
            setDataChanged();
            MapAction *action = new SingleRoomAction(new ModifyRoomFlags(flag,
                                                                         field,
                                                                         FlagModifyMode::TOGGLE),
                                                     room->getId());
            scheduleAction(action);
        }
    }
}

bool MapData::getRoomFlag(const Coordinate &pos, const uint flag, const RoomField field)
{
    const QVariant val = getRoomField(pos, field);
    return !val.isNull() && IS_SET(val.toUInt(), flag);
}

void MapData::setRoomField(const Coordinate &pos, const QVariant &flag, const RoomField field)
{
    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (field < RoomField::LAST) {
            setDataChanged();
            MapAction *action = new SingleRoomAction(new UpdateRoomField(flag, field),
                                                     room->getId());
            scheduleAction(action);
        }
    }
}

QVariant MapData::getRoomField(const Coordinate &pos, const RoomField field)
{
    QMutexLocker locker(&mapLock);
    /* REVISIT: is it wise to just ignore bad data here? */
    if (field < RoomField::LAST) {
        if (Room *const room = map.get(pos)) {
            return room->at(field);
        }
    }
    return QVariant();
}

QList<Coordinate> MapData::getPath(const QList<CommandIdType> &dirs)
{
    QMutexLocker locker(&mapLock);
    QList<Coordinate> ret;

    //* NOTE: room is used and then reassigned inside the loop.
    if (Room *room = map.get(m_position)) {
        QListIterator<CommandIdType> iter(dirs);
        while (iter.hasNext()) {
            const CommandIdType cmd = iter.next();
            if (!isDirectionNESWUD(cmd)) {
                break;
            }

            Exit &e = room->exit(getDirection(cmd));
            if (!e.isExit()) {
                // REVISIT: why does this continue but all of the others break?
                continue;
            }

            if (!e.outIsUnique()) {
                break;
            }

            // WARNING: room is reassigned here!
            room = roomIndex[e.outFirst()];
            if (room == nullptr) {
                break;
            }
            ret.append(room->getPosition());
        }
    }
    return ret;
}

const SigRoomSelection MapData::select(const Coordinate &ulf, const Coordinate &lrb)
{
    QMutexLocker locker(&mapLock);
    const auto &sig = select();
    const auto &selection = sig.getShared();
    lookingForRooms(*selection, ulf, lrb);
    return sig;
}

// updates a selection created by the mapdata
const SigRoomSelection MapData::select(const Coordinate &ulf,
                                       const Coordinate &lrb,
                                       const SigRoomSelection &in)
{
    QMutexLocker locker(&mapLock);
    auto &selection = in.getShared();
    lookingForRooms(*selection, ulf, lrb);
    return in;
}
// creates and registers a selection with one room
const SigRoomSelection MapData::select(const Coordinate &pos)
{
    QMutexLocker locker(&mapLock);
    const auto &sig = select();
    const auto &selection = sig.getShared();
    lookingForRooms(*selection, pos);
    return sig;
}
// creates and registers an empty selection
const SigRoomSelection MapData::select()
{
    QMutexLocker locker(&mapLock);
    const auto selection = std::make_shared<RoomSelection>(this);
    return SigRoomSelection{selection};
}

// removes the selection from the internal structures
void MapData::unselect(RoomSelection *selection)
{
    QMutexLocker locker(&mapLock);
    QMap<RoomId, const Room *>::iterator i = selection->begin();
    while (i != selection->end()) {
        keepRoom(*selection, (i++).key());
    }
}

// the room will be inserted in the given selection. the selection must have been created by mapdata
const Room *MapData::getRoom(const Coordinate &pos, const SigRoomSelection &in)
{
    QMutexLocker locker(&mapLock);
    const auto &selection = in.getShared();
    if (Room *const room = map.get(pos)) {
        auto id = room->getId();
        locks[id].insert(selection.get());
        selection->insert(id, room);
        return room;
    }
    return nullptr;
}

const Room *MapData::getRoom(const RoomId id, const SigRoomSelection &in)
{
    QMutexLocker locker(&mapLock);
    const auto &selection = in.getShared();
    if (Room *const room = roomIndex[id]) {
        // REVISIT: is roomID the same as id?
        // Is it okay to assert(roomId == id);
        const RoomId roomId = room->getId();

        locks[roomId].insert(selection.get());
        selection->insert(roomId, room);
        return room;
    }
    return nullptr;
}

void MapData::draw(const Coordinate &ulf, const Coordinate &lrb, MapCanvasRoomDrawer &screen)
{
    QMutexLocker locker(&mapLock);
    DrawStream drawer(screen, roomIndex, locks);
    map.getRooms(drawer, ulf, lrb);
}

bool MapData::isMovable(const Coordinate &offset, const SigRoomSelection &in)
{
    QMutexLocker locker(&mapLock);
    const auto &selection = in.getShared();
    QMap<RoomId, const Room *>::const_iterator i = selection->begin();

    while (i != selection->end()) {
        const Coordinate target = (*i++)->getPosition() + offset;
        if (Room *const other = map.get(target)) {
            if (!selection->contains(other->getId())) {
                return false;
            }
        }
    }
    return true;
}

void MapData::unselectRoom(RoomId id, const SigRoomSelection &in)
{
    QMutexLocker locker(&mapLock);
    const auto &selection = in.getShared();
    keepRoom(*selection, id);
    selection->remove(id);
}

bool MapData::execute(MapAction *const action, const SigRoomSelection &unlock)
{
    QMutexLocker locker(&mapLock);
    action->schedule(this);
    std::list<RoomId> selectedIds;

    const auto &selection = unlock.getShared();
    for (auto i = selection->begin(); i != selection->end();) {
        const Room *room = *i++;
        const auto id = room->getId();
        locks[id].erase(selection.get());
        selectedIds.push_back(id);
    }
    selection->clear();

    const bool executable = isExecutable(action);
    if (executable) {
        executeAction(action);
    } else {
        qWarning() << "Unable to execute action" << action;
    }

    for (auto id : selectedIds) {
        if (Room *const room = roomIndex[id]) {
            locks[id].insert(selection.get());
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

    const auto noName = DoorName{};
    for (auto &room : roomIndex) {
        if (room != nullptr) {
            for (const auto dir : ALL_EXITS_NESWUD) {
                MapAction *action = new SingleRoomAction(new UpdateExitField(noName, dir),
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
        if (room != nullptr && f.filter(room)) {
            locks[room->getId()].insert(recipient);
            recipient->receiveRoom(this, room);
        }
    }
}

void MapData::genericSearch(const SigRoomSelection &in, const RoomFilter &f)
{
    QMutexLocker locker(&mapLock);
    const auto &selection = in.getShared();
    genericSearch(dynamic_cast<RoomRecipient *>(selection.get()), f);
}

MapData::~MapData()
{
    while (!m_markers.isEmpty()) {
        delete m_markers.takeFirst();
    }
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
