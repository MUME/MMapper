// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MmpMapStorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <QString>
#include <QXmlStreamWriter>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/enums.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "abstractmapstorage.h"
#include "basemapsavefilter.h"
#include "progresscounter.h"
#include "roomsaver.h"

MmpMapStorage::MmpMapStorage(MapData &mapdata,
                             const QString &filename,
                             QFile *const file,
                             QObject *parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
{}

MmpMapStorage::~MmpMapStorage() = default;

void MmpMapStorage::newData()
{
    qWarning() << "MmpMapStorage does not implement newData()";
}

bool MmpMapStorage::loadData()
{
    return false;
}

bool MmpMapStorage::mergeData()
{
    return false;
}

static QString getTerrainTypeName(const RoomTerrainEnum x)
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
        CASE2(DEATHTRAP, "Deathtrap");
    }
    return QString::asprintf("(TerrainType)%d", static_cast<int>(x));
#undef CASE2
}

static QString getTerrainTypeColor(const RoomTerrainEnum x)
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
        CASE2(DEATHTRAP, "0");
    }
    return "0";
#undef CASE2
}

static QString toMmpRoomId(const RoomId &roomId)
{
    return QString("%1").arg(roomId.asUint32() + 1);
}

bool MmpMapStorage::saveData(bool baseMapOnly)
{
    emit log("MmpMapStorage", "Writing data to file ...");

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    // The RoomSaver acts as a lock on the rooms.
    ConstRoomList roomList;
    RoomSaver saver(m_mapData, roomList);
    for (uint i = 0; i < m_mapData.getRoomsCount(); ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }

    uint roomsCount = saver.getRoomsCount();

    auto &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(roomsCount + 3);

    BaseMapSaveFilter filter;
    if (baseMapOnly) {
        filter.setMapData(&m_mapData);
        progressCounter.increaseTotalStepsBy(filter.prepareCount());
        filter.prepare(progressCounter);
    }

    QXmlStreamWriter stream(m_file);
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
    auto saveOne = [&stream](const Room &room) { saveRoom(room, stream); };

    for (const auto &pRoom : roomList) {
        filter.visitRoom(deref(pRoom), baseMapOnly, saveOne);
        progressCounter.step();
    }
    stream.writeEndElement(); // end rooms

    // save environments
    stream.writeStartElement("environments");
    for (auto terrainType : ALL_TERRAIN_TYPES) {
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

    emit log("MmpMapStorage", "Writing data finished.");

    m_mapData.unsetDataChanged();
    emit onDataSaved();

    return true;
}

void MmpMapStorage::saveRoom(const Room &room, QXmlStreamWriter &stream)
{
    stream.writeStartElement("room");
    stream.writeAttribute("id", toMmpRoomId(room.getId()));
    stream.writeAttribute("area", "1");
    stream.writeAttribute("title", room.getName().toQString());
    stream.writeAttribute("environment", QString("%1").arg(static_cast<int>(room.getTerrainType())));
    if (room.getLoadFlags().contains(RoomLoadFlagEnum::ATTENTION))
        stream.writeAttribute("important", "1");

    stream.writeStartElement("coord");
    const Coordinate &pos = room.getPosition();
    stream.writeAttribute("x", QString("%1").arg(pos.x));
    stream.writeAttribute("y", QString("%1").arg(pos.y));
    stream.writeAttribute("z", QString("%1").arg(pos.z));
    stream.writeEndElement(); // end coord

    for (auto dir : ALL_EXITS_NESWUD) {
        const Exit &e = room.exit(dir);
        if (e.isExit() && !e.outIsEmpty()) {
            stream.writeStartElement("exit");
            stream.writeAttribute("direction", lowercaseDirection(dir));
            // REVISIT: Can MMP handle multiple exits in the same direction?
            stream.writeAttribute("target", toMmpRoomId(e.outFirst()));
            if (e.isHiddenExit())
                stream.writeAttribute("hidden", "1");
            if (e.isDoor()) {
                stream.writeAttribute("door", "2");
            }
            stream.writeEndElement(); // end exit
        }
    }

    stream.writeEndElement(); // end room
}
