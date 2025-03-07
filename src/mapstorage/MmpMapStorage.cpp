// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MmpMapStorage.h"

#include "../global/progresscounter.h"
#include "../global/utils.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFlags.h"
#include "../map/coordinate.h"
#include "../map/enums.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "abstractmapstorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>

#include <QString>
#include <QXmlStreamWriter>

MmpMapStorage::MmpMapStorage(const AbstractMapStorage::Data &data, QObject *parent)
    : AbstractMapStorage{data, parent}
{}

MmpMapStorage::~MmpMapStorage() = default;

NODISCARD static QString getTerrainTypeName(const RoomTerrainEnum x)
{
#define CASE2(UPPER, PrettyName) \
    do { \
    case RoomTerrainEnum::UPPER: \
        return QString{PrettyName}; \
    } while (false)
    switch (x) {
        CASE2(UNDEFINED, "Undefined");
        CASE2(INDOORS, "Indoors");
        CASE2(CITY, "City");
        CASE2(FIELD, "Field");
        CASE2(FOREST, "Forest");
        CASE2(HILLS, "Hills");
        CASE2(MOUNTAINS, "Mountains");
        CASE2(SHALLOW, "Shallow");
        CASE2(WATER, "Water");
        CASE2(RAPIDS, "Rapids");
        CASE2(UNDERWATER, "Underwater");
        CASE2(ROAD, "Road");
        CASE2(BRUSH, "Brush");
        CASE2(TUNNEL, "Tunnel");
        CASE2(CAVERN, "Cavern");
    }
    return QString::asprintf("(TerrainType)%d", static_cast<int>(x));
#undef CASE2
}

NODISCARD static QString getTerrainTypeColor(const RoomTerrainEnum x)
{
#define CASE2(UPPER, Color) \
    do { \
    case RoomTerrainEnum::UPPER: \
        return QString{Color}; \
    } while (false)
    switch (x) {
        CASE2(UNDEFINED, "0");
        CASE2(INDOORS, "8");
        CASE2(CITY, "7");
        CASE2(FIELD, "10");
        CASE2(FOREST, "2");
        CASE2(HILLS, "3");
        CASE2(MOUNTAINS, "1");
        CASE2(SHALLOW, "14");
        CASE2(WATER, "12");
        CASE2(RAPIDS, "4");
        CASE2(UNDERWATER, "4");
        CASE2(ROAD, "11");
        CASE2(BRUSH, "6");
        CASE2(TUNNEL, "8");
        CASE2(CAVERN, "8");
    }
    return "0";
#undef CASE2
}

NODISCARD static QString toMmpRoomId(const ExternalRoomId &roomId)
{
    // Not to be confused with roomId.next().asUint32()
    const auto serialId = roomId.asUint32() + 1;
    return QString("%1").arg(serialId);
}

bool MmpMapStorage::virt_saveData(const RawMapData &mapData)
{
    log("Writing data to file ...");

    const auto &map = mapData.mapPair.modified;

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.

    ConstRoomList roomList;
    roomList.reserve(map.getRoomsCount());
    for (const RoomId id : map.getRooms()) {
        const auto &room = map.getRoomHandle(id);
        if (!room.isTemporary()) {
            roomList.emplace_back(room);
        }
    }

    const auto roomsCount = static_cast<uint32_t>(roomList.size());

    auto &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(roomsCount + 3);

    QXmlStreamWriter stream(getFile());
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    // save map
    stream.writeStartElement("map");

    // save areas
    stream.writeStartElement("areas");
    stream.writeStartElement("area");
    stream.writeAttribute("id", "1");
    stream.writeAttribute("name", "Arda");
    stream.writeEndElement(); // end area
    stream.writeEndElement(); // end areas
    progressCounter.step();

    // save rooms
    stream.writeStartElement("rooms");
    for (const RoomPtr &pRoom : roomList) {
        saveRoom(pRoom->getRawCopyExternal(), stream);
        progressCounter.step();
    }
    stream.writeEndElement(); // end rooms

    // save environments
    stream.writeStartElement("environments");
    for (const RoomTerrainEnum terrainType : ALL_TERRAIN_TYPES) {
        stream.writeStartElement("environment");
        stream.writeAttribute("id", QString("%1").arg(static_cast<int>(terrainType)));
        stream.writeAttribute("name", getTerrainTypeName(terrainType));
        stream.writeAttribute("color", getTerrainTypeColor(terrainType));
        stream.writeEndElement(); // end environment
    }
    stream.writeEndElement(); // end environments
    progressCounter.step();

    stream.writeEndElement(); // end map
    progressCounter.step();

    log("Writing data finished.");

    return true;
}

void MmpMapStorage::saveRoom(const ExternalRawRoom &room, QXmlStreamWriter &stream)
{
    stream.writeStartElement("room");
    stream.writeAttribute("id", toMmpRoomId(room.getId()));
    stream.writeAttribute("area", "1");
    stream.writeAttribute("title", room.getName().toQString());
    stream.writeAttribute("environment", QString("%1").arg(static_cast<int>(room.getTerrainType())));
    if (room.getLoadFlags().contains(RoomLoadFlagEnum::ATTENTION)
        || room.getLoadFlags().contains(RoomLoadFlagEnum::DEATHTRAP)) {
        stream.writeAttribute("important", "1");
    }

    stream.writeStartElement("coord");
    const Coordinate &pos = room.getPosition();
    stream.writeAttribute("x", QString("%1").arg(pos.x));
    stream.writeAttribute("y", QString("%1").arg(pos.y));
    stream.writeAttribute("z", QString("%1").arg(pos.z));
    stream.writeEndElement(); // end coord

    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const auto &e = room.getExit(dir);
        if (e.exitIsExit() && !e.outIsEmpty()) {
            stream.writeStartElement("exit");
            stream.writeAttribute("direction", lowercaseDirection(dir));
            // REVISIT: Can MMP handle multiple exits in the same direction?
            stream.writeAttribute("target", toMmpRoomId(e.outFirst()));
            if (e.doorIsHidden()) {
                stream.writeAttribute("hidden", "1");
            }
            if (e.exitIsDoor()) {
                stream.writeAttribute("door", "2");
            }
            stream.writeEndElement(); // end exit
        }
    }

    stream.writeEndElement(); // end room
}
