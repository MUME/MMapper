// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <QMessageBox>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mainwindow/UpdateDialog.h" // CompareVersion
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
    , roomIds()
    , toRoomIds()
{}

XmlMapStorage::~XmlMapStorage() = default;

void XmlMapStorage::newData()
{
    qWarning() << "XmlMapStorage does not implement newData()";
}

bool XmlMapStorage::mergeData()
{
    return false;
}

// --------------------------- loadData ----------------------------------------
bool XmlMapStorage::loadData()
{
    // clear previous map
    m_mapData.clear();
    try {
        log("Loading data ...");
        QXmlStreamReader stream(m_file);
        loadWorld(stream);
        log("Finished loading.");

        m_mapData.checkSize();
        m_mapData.setFileName(m_fileName, true);

        emit sig_onDataLoaded();
        return true;
    } catch (const std::exception &ex) {
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        log(msg);
        qWarning().noquote() << msg;

        QMessageBox::critical(checked_dynamic_downcast<QWidget *>(parent()),
                              tr("XmlMapStorage Error"),
                              msg);

        m_mapData.clear();
        return false;
    }
}

void XmlMapStorage::loadWorld(QXmlStreamReader &stream)
{
    ProgressCounter &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(32728); // just a guess

    MapFrontendBlocker blocker(m_mapData);
    m_mapData.setDataChanged();

    roomIds.clear();
    toRoomIds.clear();

    while (stream.readNextStartElement() && !stream.hasError()) {
        if (stream.name() == "map") {
            loadMap(stream);
        }
        if (stream.tokenType() != QXmlStreamReader::EndElement) {
            stream.skipCurrentElement();
        }
    }
}

// load current <map> element
void XmlMapStorage::loadMap(QXmlStreamReader &stream)
{
    {
        const QXmlStreamAttributes attrs = stream.attributes();
        const QString type = attrs.value("type").toString();
        if (type != "mmapper2xml") {
            throwErrorFmt("This mm2xml map has type=\"%1\", expecting type=\"mmapper2xml\"", type);
        }
        const QString version = attrs.value("version").toString();
        const CompareVersion cmp(version);
        if (cmp.major() != 1) {
            throwErrorFmt("This mm2xml map has version=\"%1\", expecting version=\"1.x.y\"",
                          version);
        }
    }
    ProgressCounter &progressCounter = getProgressCounter();

    while (stream.readNextStartElement() && !stream.hasError()) {
        if (stream.name() == "room") {
            loadRoom(stream);
            progressCounter.step();
        }
        if (stream.tokenType() != QXmlStreamReader::EndElement) {
            stream.skipCurrentElement();
        }
    }
}

// load current <room> element
void XmlMapStorage::loadRoom(QXmlStreamReader &stream)
{
    const SharedRoom room = Room::createPermanentRoom(m_mapData);

    const QXmlStreamAttributes attrs = stream.attributes();
    const QStringRef idstr = attrs.value("id");
    const RoomId roomId(idstr.toUInt());
    if (idstr != QString("%1").arg(roomId.asUint32())) {
        throwErrorFmt("invalid room id=\"%1\"", idstr);
    } else if (!roomIds.insert(roomId).second) {
        throwErrorFmt("duplicated room id=\"%1\"", idstr);
    }
    room->setId(roomId);
    if (attrs.value("uptodate") == "false") {
        room->setOutDated();
    } else {
        room->setUpToDate();
    }
    room->setName(RoomName{attrs.value("name").toString()});

    while (stream.readNextStartElement() && !stream.hasError()) {
        const QStringRef name = stream.name();
        qDebug() << "loadRoom: found " << name;
        if (name == "align") {
            room->setAlignType(loadAlign(stream));
        } else if (name == "coord") {
            room->setPosition(loadCoordinate(stream));
        } else if (name == "light") {
        } else if (name == "loadflag") {
        } else if (name == "mobflag") {
        } else if (name == "portable") {
        } else if (name == "ridable") {
        } else if (name == "sundeath") {
        } else if (name == "terrain") {
        } else {
        }
        if (stream.tokenType() != QXmlStreamReader::EndElement) {
            stream.skipCurrentElement();
        }
    }
    m_mapData.insertPredefinedRoom(room);
}

// load current <align> element
RoomAlignEnum XmlMapStorage::loadAlign(QXmlStreamReader &stream)
{
    QStringRef align;
    RoomAlignEnum e = RoomAlignEnum::UNDEFINED;
    bool fail = stream.readNext() != QXmlStreamReader::Characters;
    if (!fail) {
        align = stream.text();
        qDebug() << "loadAlign: found " << align;
        e = toEnum<RoomAlignEnum>(align, fail);
    }
    if (fail) {
        throwErrorFmt("invalid <align>%1</align>", align);
    }
    return e;
}

// load current <coord> element
Coordinate XmlMapStorage::loadCoordinate(QXmlStreamReader &stream)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    bool fail = false;
    const int x = toNumber<int>(attrs.value("x"), fail);
    const int y = toNumber<int>(attrs.value("y"), fail);
    const int z = toNumber<int>(attrs.value("z"), fail);
    if (fail) {
        throwErrorFmt("invalid coordinate x=\"%1\" y=\"%2\" z=\"%3\"",
                      attrs.value("x"),
                      attrs.value("y"),
                      attrs.value("z"));
    }
    return Coordinate(x, y, z);
}

// currently leaks memory due to new QString(...) - not a big issue since it's a static member
const std::vector<std::unordered_map<QStringRef, uint>> XmlMapStorage::stringToEnumMap{
#define DECL(X) {new QString(#X), uint(RoomAlignEnum::X)},
    {X_FOREACH_RoomAlignEnum(DECL)},
#undef DECL
#define DECL(X, ...) {new QString(#X), uint(DoorFlagEnum::X)},
    {X_FOREACH_DOOR_FLAG(DECL)},
#undef DECL
#define DECL(X, ...) {new QString(#X), uint(ExitFlagEnum::X)},
    {X_FOREACH_EXIT_FLAG(DECL)},
#undef DECL
#define DECL(X, ...) {new QString(#X), uint(ExitFlagEnum::X)},
    {X_FOREACH_EXIT_FLAG(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomLightEnum::X)},
    {X_FOREACH_RoomLightEnum(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomLoadFlagEnum::X)},
    {X_FOREACH_ROOM_LOAD_FLAG(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(InfoMarkClassEnum::X)},
    {X_FOREACH_INFOMARK_CLASS(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(InfoMarkTypeEnum::X)},
    {X_FOREACH_INFOMARK_TYPE(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomMobFlagEnum::X)},
    {X_FOREACH_ROOM_MOB_FLAG(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomPortableEnum::X)},
    {X_FOREACH_RoomPortableEnum(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomRidableEnum::X)},
    {X_FOREACH_RoomRidableEnum(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomSundeathEnum::X)},
    {X_FOREACH_RoomSundeathEnum(DECL)},
#undef DECL
#define DECL(X) {new QString(#X), uint(RoomTerrainEnum::X)},
    {X_FOREACH_RoomTerrainEnum(DECL)},
#undef DECL
};

uint XmlMapStorage::stringToEnum(uint index, const QStringRef &str, bool &fail)
{
    if (index < stringToEnumMap.size()) {
        auto iter = stringToEnumMap[index].find(str);
        if (iter != stringToEnumMap[index].end()) {
            return iter->second;
        }
    }
    fail = true;
    return 0;
}

// --------------------------- saveData ----------------------------------------
bool XmlMapStorage::saveData(bool baseMapOnly)
{
    log("Writing data to file ...");
    QXmlStreamWriter stream(m_file);
    saveWorld(stream, baseMapOnly);
    log("Writing data finished.");

    m_mapData.unsetDataChanged();
    emit sig_onDataSaved();
    return true;
}

void XmlMapStorage::saveWorld(QXmlStreamWriter &stream, bool baseMapOnly)
{
    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    // The RoomSaver acts as a lock on the rooms.
    const quint32 roomsCount = m_mapData.getRoomsCount();

    ConstRoomList roomList;
    roomList.reserve(roomsCount);

    RoomSaver saver(m_mapData, roomList);
    for (quint32 i = 0; i < roomsCount; ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }
    const MarkerList &markerList = m_mapData.getMarkersList();

    ProgressCounter &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(saver.getRoomsCount()
                                         + static_cast<quint32>(markerList.size()));

    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("map");
    saveXmlAttribute(stream, "type", "mmapper2xml");
    saveXmlAttribute(stream, "version", "1.0.0");

    saveRooms(stream, baseMapOnly, roomList);
    saveMarkers(stream, markerList);
    // write selected room x,y,z
    saveCoordinate(stream, "position", m_mapData.getPosition());

    stream.writeEndElement(); // end map
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

    saveXmlAttribute(stream, "id", QString("%1").arg(room.getId().asUint32()));
    saveXmlAttribute(stream, "name", room.getName().toQString());
    if (!room.isUpToDate()) {
        saveXmlAttribute(stream, "uptodate", "false");
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
    saveXmlAttribute(stream, "x", QString("%1").arg(pos.x));
    saveXmlAttribute(stream, "y", QString("%1").arg(pos.y));
    saveXmlAttribute(stream, "z", QString("%1").arg(pos.z));
    stream.writeEndElement(); // end coordinate
}

void XmlMapStorage::saveExit(QXmlStreamWriter &stream, const Exit &e, ExitDirEnum dir)
{
    if (!e.exitIsExit() || e.outIsEmpty()) {
        return;
    }
    stream.writeStartElement("exit");
    saveXmlAttribute(stream, "dir", lowercaseDirection(dir));
    saveXmlAttribute(stream, "doorname", e.getDoorName().toQString());
    saveExitTo(stream, e);
    saveDoorFlags(stream, e.getDoorFlags());
    saveExitFlags(stream, e.getExitFlags());
    stream.writeEndElement(); // end exit
}

void XmlMapStorage::saveExitTo(QXmlStreamWriter &stream, const Exit &e)
{
    for (const RoomId id : e.outRange()) {
        saveXmlElement(stream, "to", QString("%1").arg(id.asUint32()));
    }
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
    saveXmlAttribute(stream, "type", markTypeName(type));
    saveXmlAttribute(stream, "class", markClassName(marker.getClass()));
    // REVISIT: round to 45 degrees?
    if (marker.getRotationAngle() != 0) {
        saveXmlAttribute(stream,
                         "angle",
                         QString("%1").arg(static_cast<qint32>(marker.getRotationAngle())));
    }
    saveCoordinate(stream, "pos1", marker.getPosition1());
    saveCoordinate(stream, "pos2", marker.getPosition2());

    if (type == InfoMarkTypeEnum::TEXT) {
        saveXmlElement(stream, "text", marker.getText().toQString());
    }

    stream.writeEndElement(); // end marker
}

void XmlMapStorage::saveXmlElement(QXmlStreamWriter &stream,
                                   const QString &name,
                                   const QString &value)
{
    if (!value.isEmpty()) {
        stream.writeTextElement(name, value);
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
    for (const DoorFlagEnum e : ALL_DOOR_FLAGS) {
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
    for (const ExitFlagEnum e : ALL_EXIT_FLAGS) {
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
    for (const RoomLoadFlagEnum e : ALL_LOAD_FLAGS) {
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
    for (const RoomMobFlagEnum e : ALL_MOB_FLAGS) {
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

void XmlMapStorage::throwError(const QString &msg)
{
    throw std::runtime_error(msg.toUtf8().toStdString());
}
