// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

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

XmlMapStorage::XmlMapStorage(MapData &mapdata,
                             const QString &filename,
                             QFile *const file,
                             QObject *parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
{}

XmlMapStorage::~XmlMapStorage() = default;

void XmlMapStorage::newData()
{
    qWarning() << "XmlMapStorage does not implement newData()";
}

bool XmlMapStorage::loadData()
{
    return false;
}

bool XmlMapStorage::mergeData()
{
    return false;
}

bool XmlMapStorage::saveData(bool baseMapOnly)
{
    log("Writing data to file ...");

    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    // The RoomSaver acts as a lock on the rooms.
    ConstRoomList roomList;
    roomList.reserve(m_mapData.getRoomsCount());

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
    stream.writeAttribute("type", "mmapper2xml");
    stream.writeAttribute("version", "1");

    // save rooms
    stream.writeStartElement("rooms");
    auto saveOne = [&stream](const Room &room) { saveRoom(stream, room); };

    for (const auto &pRoom : roomList) {
        filter.visitRoom(deref(pRoom), baseMapOnly, saveOne);
        progressCounter.step();
    }
    stream.writeEndElement(); // end rooms

    progressCounter.step();

    stream.writeEndElement(); // end map
    progressCounter.step();

    log("Writing data finished.");

    m_mapData.unsetDataChanged();
    emit sig_onDataSaved();

    return true;
}

void XmlMapStorage::saveRoom(QXmlStreamWriter &stream, const Room &room)
{
    stream.writeStartElement("room");
    stream.writeAttribute("id", QString("%1").arg(room.getId().asUint32()));
    stream.writeAttribute("name", room.getName().toQString());
    saveXmlElement(stream, "terrain", terrainName(room.getTerrainType()));
    saveXmlElement(stream, "light", lightName(room.getLightType()));
    saveXmlElement(stream, "ridable", ridableName(room.getRidableType()));
    saveXmlElement(stream, "sundeath", sundeathName(room.getSundeathType()));
    saveXmlElement(stream, "align", alignName(room.getAlignType()));
    saveRoomFlags(stream, room.getLoadFlags());

    stream.writeStartElement("coord");
    const Coordinate &pos = room.getPosition();
    stream.writeAttribute("x", QString("%1").arg(pos.x));
    stream.writeAttribute("y", QString("%1").arg(pos.y));
    stream.writeAttribute("z", QString("%1").arg(pos.z));
    stream.writeEndElement(); // end coord

    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const Exit &e = room.exit(dir);
        if (e.exitIsExit() && !e.outIsEmpty()) {
            stream.writeStartElement("exit");
            stream.writeAttribute("direction", lowercaseDirection(dir));
            // REVISIT: Can MMP handle multiple exits in the same direction?
            stream.writeAttribute("target", QString("%1").arg(e.outFirst().asUint32()));
            if (e.isHiddenExit())
                stream.writeAttribute("hidden", "true");
            if (e.exitIsDoor()) {
                stream.writeAttribute("door", "true");
            }
            stream.writeEndElement(); // end exit
        }
    }
    saveXmlElement(stream, "description", room.getDescription().toQString());
    saveXmlElement(stream, "contents", room.getContents().toQString());
    saveXmlElement(stream, "note", room.getNote().toQString());

    stream.writeEndElement(); // end room
}

void XmlMapStorage::saveXmlAttribute(QXmlStreamWriter &stream,
                                     const QString &name,
                                     const QString &value)
{
    if (!value.isEmpty()) {
        stream.writeAttribute(name, value);
    }
}

void XmlMapStorage::saveXmlElement(QXmlStreamWriter &stream,
                                   const QString &name,
                                   const QString &value)
{
    if (!value.isEmpty()) {
        stream.writeStartElement(name);
        stream.writeCharacters(value);
        stream.writeEndElement(); // end description
    }
}

#define DECL(X) #X,
static const char *const flagNames[] = {
    X_FOREACH_ROOM_LOAD_FLAG(DECL) //
};
static const char *const terrainNames[] = {
    X_FOREACH_RoomTerrainEnum(DECL) //
};
static const char *const lightNames[] = {
    X_FOREACH_RoomLightEnum(DECL) //
};
static const char *const portableNames[] = {
    X_FOREACH_RoomPortableEnum(DECL) //
};
static const char *const ridableNames[] = {
    X_FOREACH_RoomRidableEnum(DECL) //
};
static const char *const sundeathNames[] = {
    X_FOREACH_RoomSundeathEnum(DECL) //
};
static const char *const alignNames[] = {
    X_FOREACH_RoomAlignEnum(DECL) //
};

#undef DECL

void XmlMapStorage::saveRoomFlags(QXmlStreamWriter &stream, RoomLoadFlags fl)
{
    if (fl.isEmpty()) {
        return;
    }
    stream.writeStartElement("flags");
    QString separator;
    for (uint x = 0; x < sizeof(flagNames) / sizeof(flagNames[0]); x++) {
        if (fl.contains(RoomLoadFlagEnum(x))) {
            stream.writeCharacters(separator);
            if (separator.isEmpty()) {
                separator = ",";
            }
            stream.writeCharacters(flagNames[x]);
        }
    }
    stream.writeEndElement();
}

NODISCARD QString XmlMapStorage::terrainName(const RoomTerrainEnum x)
{
    if (x == RoomTerrainEnum::UNDEFINED) {
        return QString();
    }
    if (uint(x) > 0 && uint(x) < sizeof(terrainNames) / sizeof(terrainNames[0])) {
        return QString{terrainNames[uint(x)]};
    }
    return QString("%1").arg(uint(x));
}

NODISCARD QString XmlMapStorage::lightName(const RoomLightEnum x)
{
    if (x == RoomLightEnum::UNDEFINED) {
        return QString();
    }
    if (uint(x) > 0 && uint(x) < sizeof(lightNames) / sizeof(lightNames[0])) {
        return QString{lightNames[uint(x)]};
    }
    return QString("%1").arg(uint(x));
}

NODISCARD QString XmlMapStorage::ridableName(const RoomRidableEnum x)
{
    if (x == RoomRidableEnum::UNDEFINED) {
        return QString();
    }
    if (uint(x) > 0 && uint(x) < sizeof(ridableNames) / sizeof(ridableNames[0])) {
        return QString{ridableNames[uint(x)]};
    }
    return QString("%1").arg(uint(x));
}

NODISCARD QString XmlMapStorage::sundeathName(const RoomSundeathEnum x)
{
    if (x == RoomSundeathEnum::UNDEFINED) {
        return QString();
    }
    if (uint(x) > 0 && uint(x) < sizeof(sundeathNames) / sizeof(sundeathNames[0])) {
        return QString{sundeathNames[uint(x)]};
    }
    return QString("%1").arg(uint(x));
}

NODISCARD QString XmlMapStorage::alignName(const RoomAlignEnum x)
{
    if (x == RoomAlignEnum::UNDEFINED) {
        return QString();
    }
    if (uint(x) > 0 && uint(x) < sizeof(alignNames) / sizeof(alignNames[0])) {
        return QString{alignNames[uint(x)]};
    }
    return QString("%1").arg(uint(x));
}
