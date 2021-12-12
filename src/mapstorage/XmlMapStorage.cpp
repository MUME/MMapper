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
    progressCounter.increaseTotalStepsBy(roomsCount + 2);

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
    auto saveOne = [&stream](const Room &room) { saveRoom(stream, room); };

    for (const auto &pRoom : roomList) {
        filter.visitRoom(deref(pRoom), baseMapOnly, saveOne);
        progressCounter.step();
    }

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
    saveXmlElement(stream, "align", alignName(room.getAlignType()));
    saveXmlElement(stream, "light", lightName(room.getLightType()));
    saveXmlElement(stream, "portable", portableName(room.getPortableType()));
    saveXmlElement(stream, "ridable", ridableName(room.getRidableType()));
    saveXmlElement(stream, "sundeath", sundeathName(room.getSundeathType()));
    saveXmlElement(stream, "terrain", terrainName(room.getTerrainType()));
    saveRoomLoadFlags(stream, room.getLoadFlags());
    saveRoomMobFlags(stream, room.getMobFlags());

    stream.writeStartElement("coord");
    const Coordinate &pos = room.getPosition();
    stream.writeAttribute("x", QString("%1").arg(pos.x));
    stream.writeAttribute("y", QString("%1").arg(pos.y));
    stream.writeAttribute("z", QString("%1").arg(pos.z));
    stream.writeEndElement(); // end coord

    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        saveRoomExit(stream, room.exit(dir), dir);
    }
    saveXmlElement(stream, "description", room.getDescription().toQString());
    saveXmlElement(stream, "contents", room.getContents().toQString());
    saveXmlElement(stream, "note", room.getNote().toQString());

    stream.writeEndElement(); // end room
}

void XmlMapStorage::saveRoomExit(QXmlStreamWriter &stream, const Exit &e, ExitDirEnum dir)
{
    if (!e.exitIsExit() || e.outIsEmpty()) {
        return;
    }
    stream.writeStartElement("exit");
    stream.writeAttribute("dir", lowercaseDirection(dir));
    saveRoomExitTo(stream, e);
    saveRoomExitFlags(stream, e.getExitFlags());
    if (e.hasDoorName()) {
        stream.writeAttribute("doorname", e.getDoorName().toQString());
    }
    stream.writeEndElement(); // end exit
}

void XmlMapStorage::saveRoomExitTo(QXmlStreamWriter &stream, const Exit &e)
{
    QString idlist;
    const char *separator = nullptr;
    for (const RoomId id : e.outRange()) {
        if (separator == nullptr) {
            separator = ",";
        } else {
            idlist += separator;
        }
        idlist += QString("%1").arg(id.asUint32());
    }
    stream.writeAttribute("to", idlist);
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

void XmlMapStorage::saveXmlAttribute(QXmlStreamWriter &stream,
                                     const QString &name,
                                     const QString &value)
{
    if (!value.isEmpty()) {
        stream.writeAttribute(name, value);
    }
}

void XmlMapStorage::log(const QString &msg)
{
    emit sig_log("XmlMapStorage", msg);
}

#define DECL(X, ...) #X,
static const char *const alignNames[] = {
    X_FOREACH_RoomAlignEnum(DECL) //
};
static const char *const exitFlagNames[] = {
    X_FOREACH_EXIT_FLAG(DECL) //
};
static const char *const lightNames[] = {
    X_FOREACH_RoomLightEnum(DECL) //
};
static const char *const loadFlagNames[] = {
    X_FOREACH_ROOM_LOAD_FLAG(DECL) //
};
static const char *const mobFlagNames[] = {
    X_FOREACH_ROOM_MOB_FLAG(DECL) //
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
static const char *const terrainNames[] = {
    X_FOREACH_RoomTerrainEnum(DECL) //
};
#undef DECL

void XmlMapStorage::saveRoomExitFlags(QXmlStreamWriter &stream, ExitFlags fl)
{
    fl.remove(ExitFlagEnum::EXIT); // always set, do not save it
    if (fl.isEmpty()) {
        return;
    }
    QString list;
    const char *separator = "";
    for (uint x = 0; x < sizeof(exitFlagNames) / sizeof(exitFlagNames[0]); x++) {
        if (fl.contains(ExitFlagEnum(x))) {
            list += separator;
            separator = ",";
            list += exitFlagNames[x];
        }
    }
    saveXmlAttribute(stream, "flags", list);
}

void XmlMapStorage::saveRoomLoadFlags(QXmlStreamWriter &stream, RoomLoadFlags fl)
{
    if (fl.isEmpty()) {
        return;
    }
    stream.writeStartElement("flags");
    QString separator;
    for (uint x = 0; x < sizeof(loadFlagNames) / sizeof(loadFlagNames[0]); x++) {
        if (fl.contains(RoomLoadFlagEnum(x))) {
            stream.writeCharacters(separator);
            if (separator.isEmpty()) {
                separator = ",";
            }
            stream.writeCharacters(loadFlagNames[x]);
        }
    }
    stream.writeEndElement();
}

void XmlMapStorage::saveRoomMobFlags(QXmlStreamWriter &stream, RoomMobFlags fl)
{
    if (fl.isEmpty()) {
        return;
    }
    stream.writeStartElement("mobflags");
    QString separator;
    for (uint x = 0; x < sizeof(mobFlagNames) / sizeof(mobFlagNames[0]); x++) {
        if (fl.contains(RoomMobFlagEnum(x))) {
            stream.writeCharacters(separator);
            if (separator.isEmpty()) {
                separator = ",";
            }
            stream.writeCharacters(mobFlagNames[x]);
        }
    }
    stream.writeEndElement();
}

NODISCARD QString XmlMapStorage::alignName(const RoomAlignEnum x)
{
    return enumName(uint(x), alignNames, sizeof(alignNames) / sizeof(alignNames[0]));
}

NODISCARD QString XmlMapStorage::lightName(const RoomLightEnum x)
{
    return enumName(uint(x), lightNames, sizeof(lightNames) / sizeof(lightNames[0]));
}

NODISCARD QString XmlMapStorage::portableName(const RoomPortableEnum x)
{
    return enumName(uint(x), portableNames, sizeof(portableNames) / sizeof(portableNames[0]));
}

NODISCARD QString XmlMapStorage::ridableName(const RoomRidableEnum x)
{
    return enumName(uint(x), ridableNames, sizeof(ridableNames) / sizeof(ridableNames[0]));
}

NODISCARD QString XmlMapStorage::sundeathName(const RoomSundeathEnum x)
{
    return enumName(uint(x), sundeathNames, sizeof(sundeathNames) / sizeof(sundeathNames[0]));
}

NODISCARD QString XmlMapStorage::terrainName(const RoomTerrainEnum x)
{
    return enumName(uint(x), terrainNames, sizeof(terrainNames) / sizeof(terrainNames[0]));
}

NODISCARD QString XmlMapStorage::enumName(const uint x,
                                          const char *const names[],
                                          const size_t count)
{
    if (x == 0) {
        return QString();
    } else if (x < count) {
        return QString(names[x]);
    } else {
        return QString("%1").arg(x);
    }
}
