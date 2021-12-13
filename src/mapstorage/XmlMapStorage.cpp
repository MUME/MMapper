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
    for (uint i = 0, n = m_mapData.getRoomsCount(); i < n; ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }
    const MarkerList &markerList = m_mapData.getMarkersList();

    ProgressCounter &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(saver.getRoomsCount()
                                         + static_cast<quint32>(markerList.size()));

    QXmlStreamWriter stream(m_file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("map");
    stream.writeAttribute("type", "mmapper2xml");
    stream.writeAttribute("version", "1.0.0");

    saveRooms(stream, baseMapOnly, roomList);
    saveMarkers(stream, markerList);
    // write selected room x,y,z
    saveCoordinate(stream, "position", m_mapData.getPosition());

    stream.writeEndElement(); // end map

    log("Writing data finished.");

    m_mapData.unsetDataChanged();
    emit sig_onDataSaved();

    return true;
}

void XmlMapStorage::saveRooms(QXmlStreamWriter &stream,
                              bool baseMapOnly,
                              const ConstRoomList &roomList)
{
    ProgressCounter &progressCounter = getProgressCounter();
    BaseMapSaveFilter filter;
    if (baseMapOnly) {
        filter.setMapData(&m_mapData);
        progressCounter.increaseTotalStepsBy(filter.prepareCount());
        filter.prepare(progressCounter);
    }
    auto saveOne = [&stream](const Room &room) { saveRoom(stream, room); };
    for (const auto &pRoom : roomList) {
        filter.visitRoom(deref(pRoom), baseMapOnly, saveOne);
        progressCounter.step();
    }
}

void XmlMapStorage::saveRoom(QXmlStreamWriter &stream, const Room &room)
{
    stream.writeStartElement("room");

    stream.writeAttribute("id", QString("%1").arg(room.getId().asUint32()));
    stream.writeAttribute("name", room.getName().toQString());
    if (!room.isUpToDate()) {
        stream.writeAttribute("uptodate", "false");
    }
    saveXmlElement(stream, "align", alignName(room.getAlignType()));
    saveXmlElement(stream, "light", lightName(room.getLightType()));
    saveXmlElement(stream, "portable", portableName(room.getPortableType()));
    saveXmlElement(stream, "ridable", ridableName(room.getRidableType()));
    saveXmlElement(stream, "sundeath", sundeathName(room.getSundeathType()));
    saveXmlElement(stream, "terrain", terrainName(room.getTerrainType()));
    saveCoordinate(stream, "coord", room.getPosition());
    saveRoomLoadFlags(stream, room.getLoadFlags());
    saveRoomMobFlags(stream, room.getMobFlags());

    for (const ExitDirEnum dir : ALL_EXITS7) {
        saveExit(stream, room.exit(dir), dir);
    }
    saveXmlElement(stream, "description", room.getDescription().toQString());
    saveXmlElement(stream, "contents", room.getContents().toQString());
    saveXmlElement(stream, "note", room.getNote().toQString());

    stream.writeEndElement(); // end room
}

void XmlMapStorage::saveCoordinate(QXmlStreamWriter &stream,
                                   const QString &name,
                                   const Coordinate &pos)
{
    stream.writeStartElement(name);
    stream.writeAttribute("x", QString("%1").arg(pos.x));
    stream.writeAttribute("y", QString("%1").arg(pos.y));
    stream.writeAttribute("z", QString("%1").arg(pos.z));
    stream.writeEndElement(); // end coord
}

void XmlMapStorage::saveExit(QXmlStreamWriter &stream, const Exit &e, ExitDirEnum dir)
{
    if (!e.exitIsExit() || e.outIsEmpty()) {
        return;
    }
    stream.writeStartElement("exit");
    stream.writeAttribute("dir", lowercaseDirection(dir));
    saveExitTo(stream, e);
    saveXmlAttribute(stream, "doorname", e.getDoorName().toQString());
    saveDoorFlags(stream, e.getDoorFlags());
    saveExitFlags(stream, e.getExitFlags());
    stream.writeEndElement(); // end exit
}

void XmlMapStorage::saveExitTo(QXmlStreamWriter &stream, const Exit &e)
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

void XmlMapStorage::saveMarkers(QXmlStreamWriter &stream, const MarkerList &markerList)
{
    ProgressCounter &progressCounter = getProgressCounter();
    for (const auto &marker : markerList) {
        saveMarker(stream, deref(marker));
        progressCounter.step();
    }
}

void XmlMapStorage::saveMarker(QXmlStreamWriter &stream, const InfoMark &marker)
{
    const InfoMarkTypeEnum type = marker.getType();
    stream.writeStartElement("marker");
    stream.writeAttribute("type", markTypeName(type));
    stream.writeAttribute("class", markClassName(marker.getClass()));
    // REVISIT: round to 45 degrees?
    if (marker.getRotationAngle() != 0) {
        stream.writeAttribute("angle",
                              QString("%1").arg(static_cast<qint32>(marker.getRotationAngle())));
    }
    saveCoordinate(stream, "pos1", marker.getPosition1());
    saveCoordinate(stream, "pos2", marker.getPosition2());

    if (type == InfoMarkTypeEnum::TEXT) {
        stream.writeStartElement("text");
        stream.writeCharacters(marker.getText().toQString());
        stream.writeEndElement(); // end text
    }

    stream.writeEndElement(); // end marker
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

void XmlMapStorage::saveDoorFlags(QXmlStreamWriter &stream, DoorFlags fl)
{
    if (fl.isEmpty()) {
        return;
    }
    for (uint x = 0; x < NUM_DOOR_FLAGS; x++) {
        const DoorFlagEnum e = DoorFlagEnum(x);
        if (fl.contains(e)) {
            saveXmlElement(stream, "doorflag", doorFlagName(e));
        }
    }
}

void XmlMapStorage::saveExitFlags(QXmlStreamWriter &stream, ExitFlags fl)
{
    fl.remove(ExitFlagEnum::EXIT); // always set, do not save it
    if (fl.isEmpty()) {
        return;
    }
    for (uint x = 0; x < NUM_EXIT_FLAGS; x++) {
        const ExitFlagEnum e = ExitFlagEnum(x);
        if (fl.contains(e)) {
            saveXmlElement(stream, "exitflag", exitFlagName(e));
        }
    }
}

void XmlMapStorage::saveRoomLoadFlags(QXmlStreamWriter &stream, RoomLoadFlags fl)
{
    if (fl.isEmpty()) {
        return;
    }
    for (uint x = 0; x < NUM_ROOM_LOAD_FLAGS; x++) {
        const RoomLoadFlagEnum e = RoomLoadFlagEnum(x);
        if (fl.contains(e)) {
            saveXmlElement(stream, "loadflag", loadFlagName(e));
        }
    }
}

void XmlMapStorage::saveRoomMobFlags(QXmlStreamWriter &stream, RoomMobFlags fl)
{
    if (fl.isEmpty()) {
        return;
    }
    for (uint x = 0; x < NUM_ROOM_LOAD_FLAGS; x++) {
        const RoomMobFlagEnum e = RoomMobFlagEnum(x);
        if (fl.contains(e)) {
            saveXmlElement(stream, "mobflag", mobFlagName(e));
        }
    }
}

NODISCARD const char *XmlMapStorage::alignName(const RoomAlignEnum e)
{
    if (e != RoomAlignEnum::UNDEFINED) {
#define CASE(X) \
    case RoomAlignEnum::X: \
        return #X;
        switch (e) {
            X_FOREACH_RoomAlignEnum(CASE)
        }
#undef CASE
        assert(false); // reachable only on invalid RoomAlignEnum
    }
    return "";
}

NODISCARD const char *XmlMapStorage::doorFlagName(const DoorFlagEnum e)
{
#define CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    case DoorFlagEnum::UPPER_CASE: \
        return #UPPER_CASE;
    switch (e) {
        X_FOREACH_DOOR_FLAG(CASE)
    }
#undef CASE
    assert(false); // reachable only on invalid DoorFlagEnum
    return "";
}

NODISCARD const char *XmlMapStorage::exitFlagName(const ExitFlagEnum e)
{
#define CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    case ExitFlagEnum::UPPER_CASE: \
        return #UPPER_CASE;
    switch (e) {
        X_FOREACH_EXIT_FLAG(CASE)
    }
#undef CASE
    assert(false); // reachable only on invalid DoorFlagEnum
    return "";
}

NODISCARD const char *XmlMapStorage::lightName(const RoomLightEnum e)
{
    if (e != RoomLightEnum::UNDEFINED) {
#define CASE(X) \
    case RoomLightEnum::X: \
        return #X;
        switch (e) {
            X_FOREACH_RoomLightEnum(CASE)
        }
#undef CASE
        assert(false); // reachable only on invalid RoomLightEnum
    }
    return "";
}

NODISCARD const char *XmlMapStorage::loadFlagName(const RoomLoadFlagEnum e)
{
#define CASE(X) \
    case RoomLoadFlagEnum::X: \
        return #X;
    switch (e) {
        X_FOREACH_ROOM_LOAD_FLAG(CASE)
    }
#undef CASE
    assert(false); // reachable only on invalid RoomLoadFlagEnum
    return "";
}

NODISCARD const char *XmlMapStorage::markClassName(const InfoMarkClassEnum e)
{
#define CASE(X) \
    case InfoMarkClassEnum::X: \
        return #X;
    switch (e) {
        X_FOREACH_INFOMARK_CLASS(CASE)
    }
#undef CASE
    assert(false); // reachable only on invalid InfoMarkClassEnum
    return "";
}

NODISCARD const char *XmlMapStorage::markTypeName(const InfoMarkTypeEnum e)
{
#define CASE(X) \
    case InfoMarkTypeEnum::X: \
        return #X;
    switch (e) {
        X_FOREACH_INFOMARK_TYPE(CASE)
    }
#undef CASE
    assert(false); // reachable only on invalid InfoMarkTypeEnum
    return "";
}

NODISCARD const char *XmlMapStorage::mobFlagName(const RoomMobFlagEnum e)
{
#define CASE(X) \
    case RoomMobFlagEnum::X: \
        return #X;
    switch (e) {
        X_FOREACH_ROOM_MOB_FLAG(CASE)
    }
#undef CASE
    assert(false); // reachable only on invalid RoomMobFlagEnum
    return "";
}

NODISCARD const char *XmlMapStorage::portableName(const RoomPortableEnum e)
{
    if (e != RoomPortableEnum::UNDEFINED) {
#define CASE(X) \
    case RoomPortableEnum::X: \
        return #X;
        switch (e) {
            X_FOREACH_RoomPortableEnum(CASE)
        }
#undef CASE
        assert(false); // reachable only on invalid RoomPortableEnum
    }
    return "";
}

NODISCARD const char *XmlMapStorage::ridableName(const RoomRidableEnum e)
{
    if (e != RoomRidableEnum::UNDEFINED) {
#define CASE(X) \
    case RoomRidableEnum::X: \
        return #X;
        switch (e) {
            X_FOREACH_RoomRidableEnum(CASE)
        }
#undef CASE
        assert(false); // reachable only on invalid RoomRidableEnum
    }
    return "";
}

NODISCARD const char *XmlMapStorage::sundeathName(const RoomSundeathEnum e)
{
    if (e != RoomSundeathEnum::UNDEFINED) {
#define CASE(X) \
    case RoomSundeathEnum::X: \
        return #X;
        switch (e) {
            X_FOREACH_RoomSundeathEnum(CASE)
        }
#undef CASE
        assert(false); // reachable only on invalid RoomSundeathEnum
    }
    return "";
}

NODISCARD const char *XmlMapStorage::terrainName(const RoomTerrainEnum e)
{
    if (e != RoomTerrainEnum::UNDEFINED) {
#define CASE(X) \
    case RoomTerrainEnum::X: \
        return #X;
        switch (e) {
            X_FOREACH_RoomTerrainEnum(CASE)
        }
#undef CASE
        assert(false); // reachable only on invalid RoomTerrainEnum
    }
    return "";
}
