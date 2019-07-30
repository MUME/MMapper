// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

QString MapData::getDoorName(const Coordinate &pos, const ExitDirEnum dir)
{
    // REVISIT: Could this function could be made const if we make mapLock mutable?
    // Alternately, WTF are we accessing this from multiple threads?
    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirEnum::UNKNOWN) {
            return room->exit(dir).getDoorName();
        }
    }
    return "exit";
}

void MapData::setDoorName(const Coordinate &pos, const QString &name, const ExitDirEnum dir)
{
    QMutexLocker locker(&mapLock);
    const auto doorName = DoorName{name};
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirEnum::UNKNOWN) {
            setDataChanged();
            MapAction *action = new SingleRoomAction(new UpdateExitField(doorName, dir),
                                                     room->getId());
            scheduleAction(action);
        }
    }
}

bool MapData::getExitFlag(const Coordinate &pos, const ExitDirEnum dir, ExitFieldVariant var)
{
    assert(var.getType() != ExitFieldEnum::DOOR_NAME);

    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirEnum::NONE) {
            switch (var.getType()) {
            case ExitFieldEnum::DOOR_NAME: {
                const auto name = room->exit(dir).getDoorName();
                if (var.getDoorName() == name) {
                    return true;
                }
                break;
            }
            case ExitFieldEnum::EXIT_FLAGS: {
                const auto ef = room->exit(dir).getExitFlags();
                if (IS_SET(ef, var.getExitFlags())) {
                    return true;
                }
                break;
            }
            case ExitFieldEnum::DOOR_FLAGS: {
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

void MapData::toggleExitFlag(const Coordinate &pos, const ExitDirEnum dir, ExitFieldVariant var)
{
    const auto field = var.getType();
    assert(field != ExitFieldEnum::DOOR_NAME);

    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        if (dir < ExitDirEnum::NONE) {
            setDataChanged();

            MapAction *action
                = new SingleRoomAction(new ModifyExitFlags(var, dir, FlagModifyModeEnum::TOGGLE),
                                       room->getId());
            scheduleAction(action);
        }
    }
}

void MapData::toggleRoomFlag(const Coordinate &pos, RoomFieldVariant var)
{
    QMutexLocker locker(&mapLock);
    if (Room *room = map.get(pos)) {
        setDataChanged();
        // REVISIT: Consolidate ModifyRoomFlags and UpdateRoomField
        MapAction *action
            = (var.getType() == RoomFieldEnum::MOB_FLAGS
               || var.getType() == RoomFieldEnum::LOAD_FLAGS)
                  ? new SingleRoomAction(new ModifyRoomFlags(var, FlagModifyModeEnum::TOGGLE),
                                         room->getId())
                  : new SingleRoomAction(new UpdateRoomField(var), room->getId());
        scheduleAction(action);
    }
}

bool MapData::getRoomFlag(const Coordinate &pos, RoomFieldVariant var)
{
    // REVISIT: Use macros
    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        switch (var.getType()) {
        case RoomFieldEnum::NOTE: {
            return var.getNote() == room->getNote();
        }
        case RoomFieldEnum::MOB_FLAGS: {
            const auto ef = room->getMobFlags();
            if (IS_SET(ef, var.getMobFlags())) {
                return true;
            }
            break;
        }
        case RoomFieldEnum::LOAD_FLAGS: {
            const auto ef = room->getLoadFlags();
            if (IS_SET(ef, var.getLoadFlags())) {
                return true;
            }
            break;
        }
        case RoomFieldEnum::ALIGN_TYPE:
            return var.getAlignType() == room->getAlignType();
        case RoomFieldEnum::LIGHT_TYPE:
            return var.getLightType() == room->getLightType();
        case RoomFieldEnum::PORTABLE_TYPE:
            return var.getPortableType() == room->getPortableType();
        case RoomFieldEnum::RIDABLE_TYPE:
            return var.getRidableType() == room->getRidableType();
        case RoomFieldEnum::SUNDEATH_TYPE:
            return var.getSundeathType() == room->getSundeathType();
        case RoomFieldEnum::TERRAIN_TYPE:
            return var.getTerrainType() == room->getTerrainType();
        case RoomFieldEnum::NAME:
        case RoomFieldEnum::DESC:
        case RoomFieldEnum::DYNAMIC_DESC:
        case RoomFieldEnum::LAST:
        case RoomFieldEnum::RESERVED:
            throw std::runtime_error("impossible");
        }
    }
    return false;
}

QList<Coordinate> MapData::getPath(const Coordinate &start, const CommandQueue &dirs)
{
    QMutexLocker locker(&mapLock);
    QList<Coordinate> ret;

    //* NOTE: room is used and then reassigned inside the loop.
    if (Room *room = map.get(start)) {
        for (const auto cmd : dirs) {
            if (cmd == CommandEnum::LOOK)
                continue;

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

// the room will be inserted in the given selection. the selection must have been created by mapdata
const Room *MapData::getRoom(const Coordinate &pos, RoomSelection &selection)
{
    QMutexLocker locker(&mapLock);
    if (Room *const room = map.get(pos)) {
        auto id = room->getId();
        lockRoom(&selection, id);
        selection.insert(id, room);
        return room;
    }
    return nullptr;
}

const Room *MapData::getRoom(const RoomId id, RoomSelection &selection)
{
    QMutexLocker locker(&mapLock);
    if (Room *const room = roomIndex[id]) {
        const RoomId roomId = room->getId();
        assert(id == roomId);

        lockRoom(&selection, roomId);
        selection.insert(roomId, room);
        return room;
    }
    return nullptr;
}

void MapData::draw(const Coordinate &min, const Coordinate &max, MapCanvasRoomDrawer &screen)
{
    QMutexLocker locker(&mapLock);
    DrawStream drawer(screen, roomIndex, locks);
    map.getRooms(drawer, min, max);
    drawer.draw();
}

bool MapData::execute(MapAction *const action, const SharedRoomSelection &selection)
{
    QMutexLocker locker(&mapLock);
    action->schedule(this);
    std::list<RoomId> selectedIds;

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
