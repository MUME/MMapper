// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
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

// ---------------------------- XmlMapStorage::Converter -----------------------
class XmlMapStorage::Converter
{
public:
    Converter();

    // parse string containing an unsigned number.
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename T>
    static std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, T> //
    toNumber(const QStringRef &str, bool &fail)
    {
        bool ok = false;
        const ulong tmp = str.toULong(&ok);
        const T ret = static_cast<T>(tmp);
        if (!ok || static_cast<ulong>(ret) != tmp) {
            fail = true;
        }
        return ret;
    }

    // parse string containing a signed number.
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename T>
    static std::enable_if_t<std::is_integral<T>::value && !std::is_unsigned<T>::value, T> //
    toNumber(const QStringRef &str, bool &fail)
    {
        bool ok = false;
        const long tmp = str.toLong(&ok);
        const T ret = static_cast<T>(tmp);
        if (!ok || static_cast<long>(ret) != tmp) {
            fail = true;
        }
        return ret;
    }

    // parse string containing the name of an enum.
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename ENUM>
    ENUM toEnum(const QStringRef &str, bool &fail) const
    {
        static_assert(std::is_enum<ENUM>::value, "type ENUM must be an enumeration");
        return ENUM(stringToEnum(enumIndex(ENUM(0)), str, fail));
    }

    template<typename ENUM>
    const QString &toString(ENUM val) const
    {
        static_assert(std::is_enum<ENUM>::value, "type ENUM must be an enumeration");
        return enumToString(enumIndex(val), uint(val));
    }

private:
    // convert ENUM type to index in enumToString[] and stringToEnum
    static constexpr uint enumIndex(RoomAlignEnum) { return 0; }
    static constexpr uint enumIndex(DoorFlagEnum) { return 1; }
    static constexpr uint enumIndex(ExitFlagEnum) { return 2; }
    static constexpr uint enumIndex(RoomLightEnum) { return 3; }
    static constexpr uint enumIndex(RoomLoadFlagEnum) { return 4; }
    static constexpr uint enumIndex(InfoMarkClassEnum) { return 5; }
    static constexpr uint enumIndex(InfoMarkTypeEnum) { return 6; }
    static constexpr uint enumIndex(RoomMobFlagEnum) { return 7; }
    static constexpr uint enumIndex(RoomPortableEnum) { return 8; }
    static constexpr uint enumIndex(RoomRidableEnum) { return 9; }
    static constexpr uint enumIndex(RoomSundeathEnum) { return 10; }
    static constexpr uint enumIndex(RoomTerrainEnum) { return 11; }

    uint stringToEnum(uint type, const QStringRef &str, bool &fail) const;
    const QString &enumToString(uint type, uint val) const;

    std::vector<std::vector<QString>> enumToStrings;
    std::vector<std::unordered_map<QStringRef, uint>> stringToEnums;
    QString empty;
};

XmlMapStorage::Converter::Converter()
    : enumToStrings{
#define DECL(X) /* */ {#X},
#define DECL_(X, ...) {#X},
        {X_FOREACH_RoomAlignEnum(DECL)},
        {X_FOREACH_DOOR_FLAG(DECL_)},
        {X_FOREACH_EXIT_FLAG(DECL_)},
        {X_FOREACH_RoomLightEnum(DECL)},
        {X_FOREACH_ROOM_LOAD_FLAG(DECL)},
        {X_FOREACH_INFOMARK_CLASS(DECL)},
        {X_FOREACH_INFOMARK_CLASS(DECL)},
        {X_FOREACH_ROOM_MOB_FLAG(DECL)},
        {X_FOREACH_RoomPortableEnum(DECL)},
        {X_FOREACH_RoomRidableEnum(DECL)},
        {X_FOREACH_RoomSundeathEnum(DECL)},
        {X_FOREACH_RoomTerrainEnum(DECL)},
#undef DECL
#undef DECL_
    }
    , stringToEnums{}
    , empty{}
{
    // create the maps string -> enum value for each enum type listed above
    for (std::vector<QString> &vec : enumToStrings) {
        stringToEnums.emplace_back();
        std::unordered_map<QStringRef, uint> &map = stringToEnums.back();
        for (uint val = 0, n = vec.size(); val < n; val++) {
            if (vec[val] == "UNDEFINED") {
                // we never save or load the string "UNDEFINED"
                vec[val].clear();
            } else {
                map[QStringRef(&vec[val])] = val;
            }
        }
    }
}

const QString &XmlMapStorage::Converter::enumToString(uint type, uint val) const
{
    if (type < enumToStrings.size() && val < enumToStrings[type].size()) {
        return enumToStrings[type][val];
    }
    qWarning() << "Attempt to save an invalid enum (type =" << type << ", value =" << val
               << "). Either the current map is corrupted, or there is a bug";
    return empty;
}

uint XmlMapStorage::Converter::stringToEnum(uint type, const QStringRef &str, bool &fail) const
{
    if (type < stringToEnums.size()) {
        auto iter = stringToEnums[type].find(str);
        if (iter != stringToEnums[type].end()) {
            return iter->second;
        }
    }
    fail = true;
    return 0;
}

const XmlMapStorage::Converter XmlMapStorage::conv;

// ---------------------------- XmlMapStorage ----------------------------------
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

// ---------------------------- XmlMapStorage::loadData() ----------------------
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
        e = conv.toEnum<RoomAlignEnum>(align, fail);
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
    const int x = conv.toNumber<int>(attrs.value("x"), fail);
    const int y = conv.toNumber<int>(attrs.value("y"), fail);
    const int z = conv.toNumber<int>(attrs.value("z"), fail);
    if (fail) {
        throwErrorFmt("invalid coordinate x=\"%1\" y=\"%2\" z=\"%3\"",
                      attrs.value("x"),
                      attrs.value("y"),
                      attrs.value("z"));
    }
    return Coordinate(x, y, z);
}

// ---------------------------- XmlMapStorage::saveData() ----------------------
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
    saveXmlElement(stream, "align", conv.toString(room.getAlignType()));
    saveXmlElement(stream, "light", conv.toString(room.getLightType()));
    saveXmlElement(stream, "portable", conv.toString(room.getPortableType()));
    saveXmlElement(stream, "ridable", conv.toString(room.getRidableType()));
    saveXmlElement(stream, "sundeath", conv.toString(room.getSundeathType()));
    saveXmlElement(stream, "terrain", conv.toString(room.getTerrainType()));
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
    saveXmlAttribute(stream, "type", conv.toString(type));
    saveXmlAttribute(stream, "class", conv.toString(marker.getClass()));
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
            saveXmlElement(stream, "doorflag", conv.toString(e));
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
            saveXmlElement(stream, "exitflag", conv.toString(e));
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
            saveXmlElement(stream, "loadflag", conv.toString(e));
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
            saveXmlElement(stream, "mobflag", conv.toString(e));
        }
    }
}

void XmlMapStorage::throwError(const QString &msg)
{
    throw std::runtime_error(msg.toUtf8().toStdString());
}
