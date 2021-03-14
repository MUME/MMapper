// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mapdata.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <map>
#include <memory>
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
#include "roomfilter.h"
#include "roomselection.h"

MapData::MapData(QObject *const parent)
    : MapFrontend(parent)
{}

const DoorName &MapData::getDoorName(const Coordinate &pos, const ExitDirEnum dir)
{
    // REVISIT: Could this function could be made const if we make mapLock mutable?
    // Alternately, WTF are we accessing this from multiple threads?
    QMutexLocker locker(&mapLock);
    if (const Room *const room = map.get(pos)) {
        if (dir < ExitDirEnum::UNKNOWN) {
            return room->exit(dir).getDoorName();
        }
    }

    static const DoorName tmp{"exit"};
    return tmp;
}

ExitDirFlags MapData::getExitDirections(const Coordinate &pos)
{
    ExitDirFlags result;
    QMutexLocker locker(&mapLock);
    if (const Room *const room = map.get(pos)) {
        for (const ExitDirEnum dir : ALL_EXITS7) {
            if (room->exit(dir).isExit())
                result |= dir;
        }
    }
    return result;
}

bool MapData::getExitFlag(const Coordinate &pos, const ExitDirEnum dir, ExitFieldVariant var)
{
    assert(var.getType() != ExitFieldEnum::DOOR_NAME);

    QMutexLocker locker(&mapLock);
    if (const Room *const room = map.get(pos)) {
        if (dir < ExitDirEnum::NONE) {
            switch (var.getType()) {
            case ExitFieldEnum::DOOR_NAME:
                if (var.getDoorName() == room->exit(dir).getDoorName()) {
                    return true;
                }
                break;
            case ExitFieldEnum::EXIT_FLAGS:
                if (room->exit(dir).getExitFlags().containsAny(var.getExitFlags())) {
                    return true;
                }
                break;
            case ExitFieldEnum::DOOR_FLAGS:
                if (room->exit(dir).getDoorFlags().containsAny(var.getDoorFlags())) {
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

const Room *MapData::getRoom(const Coordinate &pos)
{
    QMutexLocker locker(&mapLock);
    return map.get(pos);
}

QList<Coordinate> MapData::getPath(const Coordinate &start, const CommandQueue &dirs)
{
    QMutexLocker locker(&mapLock);
    QList<Coordinate> ret;

    // NOTE: room is used and then reassigned inside the loop.
    if (const Room *room = map.get(start)) {
        for (const CommandEnum cmd : dirs) {
            if (cmd == CommandEnum::LOOK)
                continue;

            if (!isDirectionNESWUD(cmd)) {
                break;
            }

            const Exit &e = room->exit(getDirection(cmd));
            if (!e.isExit()) {
                // REVISIT: why does this continue but all of the others break?
                continue;
            }

            if (!e.outIsUnique()) {
                break;
            }

            const SharedConstRoom &tmp = roomIndex[e.outFirst()];

            // WARNING: room is reassigned here!
            room = tmp.get();

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
        selection.emplace(id, room);
        return room;
    }
    return nullptr;
}

const Room *MapData::getRoom(const RoomId id, RoomSelection &selection)
{
    QMutexLocker locker(&mapLock);
    if (const SharedRoom &room = roomIndex[id]) {
        const RoomId roomId = room->getId();
        assert(id == roomId);

        lockRoom(&selection, roomId);
        Room *const pRoom = room.get();
        selection.emplace(roomId, pRoom);
        return pRoom;
    }
    return nullptr;
}

void MapData::generateBatches(MapCanvasRoomDrawer &screen, const OptBounds &bounds)
{
    QMutexLocker locker(&mapLock);
    const LayerToRooms layerToRooms = [this]() -> LayerToRooms {
        LayerToRooms ltr;
        DrawStream drawer(ltr);
        map.getRooms(drawer);
        return ltr;
    }();
    screen.generateBatches(layerToRooms, roomIndex, bounds);
}

bool MapData::execute(std::unique_ptr<MapAction> action, const SharedRoomSelection &selection)
{
    QMutexLocker locker(&mapLock);
    action->schedule(this);
    std::list<RoomId> selectedIds;

    for (auto i = selection->begin(); i != selection->end(); i++) {
        const Room *room = i->second;
        const auto id = room->getId();
        locks[id].erase(selection.get());
        selectedIds.push_back(id);
    }
    selection->clear();

    MapAction *const pAction = action.get();
    const bool executable = isExecutable(pAction);
    if (executable) {
        executeAction(pAction);
    } else {
        qWarning() << "Unable to execute action" << pAction;
    }

    for (auto id : selectedIds) {
        if (const SharedRoom &room = roomIndex[id]) {
            locks[id].insert(selection.get());
            selection->emplace(id, room.get());
        }
    }
    return executable;
}

void MapData::virt_clear()
{
    m_markers.clear();
    log("cleared MapData");
}

void MapData::removeDoorNames()
{
    QMutexLocker locker(&mapLock);

    const auto noName = DoorName{};
    for (auto &room : roomIndex) {
        if (room != nullptr) {
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                scheduleAction(std::make_unique<SingleRoomAction>(
                    std::make_unique<ModifyExitFlags>(noName, dir, FlagModifyModeEnum::UNSET),
                    room->getId()));
            }
        }
    }
}

void MapData::genericSearch(RoomRecipient *recipient, const RoomFilter &f)
{
    QMutexLocker locker(&mapLock);
    for (const SharedRoom &room : roomIndex) {
        if (room == nullptr)
            continue;
        Room *const r = room.get();
        if (!f.filter(r))
            continue;
        locks[room->getId()].insert(recipient);
        recipient->receiveRoom(this, r);
    }
}

MapData::~MapData() = default;

void MapData::removeMarker(const std::shared_ptr<InfoMark> &im)
{
    if (im != nullptr) {
        auto it = std::find_if(m_markers.begin(), m_markers.end(), [&im](const auto &target) {
            return target == im;
        });
        if (it != m_markers.end()) {
            m_markers.erase(it);
            setDataChanged();
        }
    }
}

void MapData::removeMarkers(const MarkerList &toRemove)
{
    // If toRemove is short, this is probably "good enough." However, it may become
    // very painful if both toRemove.size() and m_markers.size() are in the thousands.
    for (const auto &im : toRemove) {
        removeMarker(im);
    }
}

void MapData::addMarker(const std::shared_ptr<InfoMark> &im)
{
    if (im != nullptr) {
        m_markers.emplace_back(im);
        setDataChanged();
    }
}
