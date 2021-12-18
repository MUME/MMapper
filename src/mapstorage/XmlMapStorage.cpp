// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <QHash>
#include <QMessageBox>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/TextUtils.h"
#include "../global/roomid.h"
#include "../global/string_view_utils.h"
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

// ---------------------------- XmlMapStorage::Type ------------------------
// list know enum types
#define X_FOREACH_TYPE_ENUM(X) \
    X(RoomAlignEnum) \
    X(DoorFlagEnum) \
    X(ExitFlagEnum) \
    X(RoomLightEnum) \
    X(RoomLoadFlagEnum) \
    X(InfoMarkClassEnum) \
    X(InfoMarkTypeEnum) \
    X(RoomMobFlagEnum) \
    X(RoomPortableEnum) \
    X(RoomRidableEnum) \
    X(RoomSundeathEnum) \
    X(RoomTerrainEnum) \
    X(Type)

enum class XmlMapStorage::Type : uint {
#define DECL(X) X,
    X_FOREACH_TYPE_ENUM(DECL)
#undef DECL
};

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
        static_assert(std::is_enum<ENUM>::value, "template type ENUM must be an enumeration");
        return ENUM(stringToEnum(enumToType(ENUM(0)), str, fail));
    }

    template<typename ENUM>
    const QString &toString(ENUM val) const
    {
        static_assert(std::is_enum<ENUM>::value, "template type ENUM must be an enumeration");
        return enumToString(enumToType(val), uint(val));
    }

private:
    // define a bunch of methods
    //   static constexpr Type enumToType(RoomAlignEnum) { return Type::RoomAlignEnum; }
    //   static constexpr Type enumToType(DoorFlagEnum)  { return Type::DoorFlagEnum;  }
    //   ...
    // converting an enumeration type to its corresponding Type value,
    // which can be used as argument in enumToString() and stringToEnum()
#define DECL(X) \
    static constexpr Type enumToType(X) { return Type::X; }
    X_FOREACH_TYPE_ENUM(DECL)
#undef DECL

    uint stringToEnum(Type type, const QStringRef &str, bool &fail) const;
    const QString &enumToString(Type type, uint val) const;

    std::vector<std::vector<QString>> enumToStrings;
    std::vector<QHash<QStringRef, uint>> stringToEnums;
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
        {X_FOREACH_TYPE_ENUM(DECL)},
#undef DECL
#undef DECL_
    }
    , stringToEnums{}
    , empty{}
{
    // create the maps string -> enum value for each enum type listed above
    for (std::vector<QString> &vec : enumToStrings) {
        stringToEnums.emplace_back();
        QHash<QStringRef, uint> &map = stringToEnums.back();
        uint val = 0;
        for (QString &str : vec) {
            if (str == "UNDEFINED") {
                str.clear(); // we never save or load the string "UNDEFINED"
            } else {
                map[QStringRef(&str)] = val;
            }
            val++;
        }
    }
}

const QString &XmlMapStorage::Converter::enumToString(Type type, uint val) const
{
    const uint index = uint(type);
    if (index < enumToStrings.size() && val < enumToStrings[index].size()) {
        return enumToStrings[index][val];
    }
    qWarning().noquote().nospace()
        << "Attempt to save an invalid enum type = " << toString(type) << ", value = " << val
        << ". Either the current map is damaged, or there is a bug";
    return empty;
}

uint XmlMapStorage::Converter::stringToEnum(Type type, const QStringRef &str, bool &fail) const
{
    const uint index = uint(type);
    if (index < stringToEnums.size()) {
        auto iter = stringToEnums[index].find(str);
        if (iter != stringToEnums[index].end()) {
            return iter.value();
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
    , loadedRooms()
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
        m_mapData.setFileName(QString(), false);

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

    while (stream.readNextStartElement() && !stream.hasError()) {
        if (stream.name() == "map") {
            loadMap(stream);
            break; // expecting only one <map>
        }
        skipXmlElement(stream);
    }
}

// load current <map> element
void XmlMapStorage::loadMap(QXmlStreamReader &stream)
{
    loadedRooms.clear();
    {
        const QXmlStreamAttributes attrs = stream.attributes();
        const QString type = attrs.value("type").toString();
        if (type != "mmapper2xml") {
            throwErrorFmt(stream,
                          "unsupported map type=\"%1\",\nexpecting type=\"mmapper2xml\"",
                          type);
        }
        const QString version = attrs.value("version").toString();
        const CompareVersion cmp(version);
        if (cmp.major() != 1) {
            throwErrorFmt(stream,
                          "unsupported map version=\"%1\",\nexpecting version=\"1.x.y\"",
                          version);
        }
    }
    ProgressCounter &progressCounter = getProgressCounter();

    while (stream.readNextStartElement() && !stream.hasError()) {
        const QStringRef name = stream.name();
        if (name == "room") {
            loadRoom(stream);
        } else if (name == "marker") {
            loadMarker(stream);
        } else if (name == "position") {
            m_mapData.setPosition(loadCoordinate(stream));
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << name << "> inside <map>";
        }
        progressCounter.step();
        skipXmlElement(stream);
    }

    connectRoomsExitFrom(stream);
    progressCounter.step();

    moveRoomsToMapData();
    progressCounter.step();
}

// load current <room> element
void XmlMapStorage::loadRoom(QXmlStreamReader &stream)
{
    const SharedRoom sharedroom = Room::createPermanentRoom(m_mapData);
    Room &room = deref(sharedroom);

    const QXmlStreamAttributes attrs = stream.attributes();
    const QStringRef idstr = attrs.value("id");
    const RoomId roomId = loadRoomId(stream, idstr);
    if (loadedRooms.count(roomId) != 0) {
        throwErrorFmt(stream, "duplicated room id \"%1\"", idstr.toString());
    }
    room.setId(roomId);
    if (attrs.value("uptodate") == "false") {
        room.setOutDated();
    } else {
        room.setUpToDate();
    }
    room.setName(RoomName{attrs.value("name").toString()});

    ExitsList exitList;
    RoomLoadFlags loadFlags;
    RoomMobFlags mobFlags;

    while (stream.readNextStartElement() && !stream.hasError()) {
        const QStringRef name = stream.name();
        if (name == "align") {
            room.setAlignType(loadEnum<RoomAlignEnum>(stream));
        } else if (name == "contents") {
            room.setContents(RoomContents{loadString(stream)});
        } else if (name == "coord") {
            room.setPosition(loadCoordinate(stream));
        } else if (name == "description") {
            room.setDescription(RoomDesc{loadString(stream)});
        } else if (name == "exit") {
            loadExit(stream, exitList);
        } else if (name == "light") {
            room.setLightType(loadEnum<RoomLightEnum>(stream));
        } else if (name == "loadflag") {
            loadFlags |= loadEnum<RoomLoadFlagEnum>(stream);
        } else if (name == "mobflag") {
            mobFlags |= loadEnum<RoomMobFlagEnum>(stream);
        } else if (name == "note") {
            room.setNote(RoomNote{loadString(stream)});
        } else if (name == "portable") {
            room.setPortableType(loadEnum<RoomPortableEnum>(stream));
        } else if (name == "ridable") {
            room.setRidableType(loadEnum<RoomRidableEnum>(stream));
        } else if (name == "sundeath") {
            room.setSundeathType(loadEnum<RoomSundeathEnum>(stream));
        } else if (name == "terrain") {
            room.setTerrainType(loadEnum<RoomTerrainEnum>(stream));
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << name << "> inside <room id=\"" << idstr << "\">";
        }
        skipXmlElement(stream);
    }
    room.setExitsList(exitList);
    room.setLoadFlags(loadFlags);
    room.setMobFlags(mobFlags);

    loadedRooms.emplace(std::move(roomId), std::move(sharedroom));
}

// convert string to RoomId
RoomId XmlMapStorage::loadRoomId(QXmlStreamReader &stream, const QStringRef &idstr)
{
    const RoomId id{idstr.toUInt()};
    // convert number back to string, and compare the two:
    // if they differ, room ID is invalid.
    if (idstr != roomIdToString(id)) {
        throwErrorFmt(stream, "invalid room id \"%1\"", idstr);
    }
    return id;
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
        throwErrorFmt(stream,
                      "invalid coordinate values x=\"%1\" y=\"%2\" z=\"%3\"",
                      attrs.value("x").toString(),
                      attrs.value("y").toString(),
                      attrs.value("z").toString());
    }
    return Coordinate(x, y, z);
}

// load current <exit> element
void XmlMapStorage::loadExit(QXmlStreamReader &stream, ExitsList &exitList)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    const ExitDirEnum dir = directionForLowercase(to_u16string_view(attrs.value("dir")));
    DoorFlags doorFlags;
    ExitFlags exitFlags;
    Exit &exit = exitList[dir];
    exit.setDoorName(DoorName{attrs.value("doorname").toString()});

    while (stream.readNextStartElement() && !stream.hasError()) {
        const QStringRef name = stream.name();
        if (name == "to") {
            exit.addOut(loadRoomId(stream, loadStringRef(stream)));
        } else if (name == "doorflag") {
            doorFlags |= loadEnum<DoorFlagEnum>(stream);
        } else if (name == "exitflag") {
            exitFlags |= loadEnum<ExitFlagEnum>(stream);
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << name << "> inside <exit>";
        }
        skipXmlElement(stream);
    }
    exit.setDoorFlags(doorFlags);
    // EXIT flag is almost always set, thus we save it inverted.
    exit.setExitFlags(exitFlags ^ ExitFlagEnum::EXIT);
}

// check that all rooms' exits "to" actually point to an existing room,
// and add matching exits "from"
void XmlMapStorage::connectRoomsExitFrom(QXmlStreamReader &stream)
{
    for (auto &elem : loadedRooms) {
        Room &room = deref(elem.second);
        for (const ExitDirEnum dir : ALL_EXITS7) {
            connectRoomExitFrom(stream, room, dir);
        }
    }
}

void XmlMapStorage::connectRoomExitFrom(QXmlStreamReader &stream,
                                        const Room &fromRoom,
                                        ExitDirEnum dir)
{
    const Exit &fromE = fromRoom.exit(dir);
    if (fromE.outIsEmpty()) {
        return;
    }
    const RoomId fromId = fromRoom.getId();
    for (const RoomId toId : fromE.outRange()) {
        auto iter = loadedRooms.find(toId);
        if (iter == loadedRooms.end()) {
            throwErrorFmt(stream,
                          "room %1 has exit %2 to non-existing room %3",
                          roomIdToString(fromId),
                          lowercaseDirectionC(dir),
                          roomIdToString(toId));
        }
        Room &toRoom = deref(iter->second);
        toRoom.addInExit(opposite(dir), fromId);
    }
}

// add all loaded rooms to m_mapData
void XmlMapStorage::moveRoomsToMapData()
{
    for (auto &elem : loadedRooms) {
        m_mapData.insertPredefinedRoom(elem.second);
    }
    loadedRooms.clear();
}

void XmlMapStorage::loadMarker(QXmlStreamReader &stream)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    bool fail = false;
    const InfoMarkTypeEnum type = conv.toEnum<InfoMarkTypeEnum>(attrs.value("type"), fail);
    const InfoMarkClassEnum clas = conv.toEnum<InfoMarkClassEnum>(attrs.value("class"), fail);
    if (fail) {
        throwErrorFmt(stream,
                      "invalid marker attributes type=\"%1\" class=\"%2\"",
                      attrs.value("type").toString(),
                      attrs.value("class").toString());
    }
    QStringRef anglestr = attrs.value("angle");
    int angle = 0;
    if (!anglestr.isNull()) {
        angle = conv.toNumber<int>(anglestr, fail);
        if (fail) {
            throwErrorFmt(stream, "invalid marker attribute angle=\"%1\"", anglestr.toString());
        }
    }

    SharedInfoMark sharedmarker = InfoMark::alloc(m_mapData);
    InfoMark &marker = deref(sharedmarker);

    marker.setType(type);
    marker.setClass(clas);
    marker.setRotationAngle(angle);

    while (stream.readNextStartElement() && !stream.hasError()) {
        const QStringRef name = stream.name();
        if (name == "pos1") {
            marker.setPosition1(loadCoordinate(stream));
        } else if (name == "pos2") {
            marker.setPosition2(loadCoordinate(stream));
        } else if (name == "text") {
            // load text only if type == TEXT
            if (type == InfoMarkTypeEnum::TEXT) {
                marker.setText(InfoMarkText{loadString(stream)});
            }
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << name << "> inside <marker>";
        }
        skipXmlElement(stream);
    }

    // REVISIT: Just discard empty text markers?
    if (type == InfoMarkTypeEnum::TEXT && marker.getText().isEmpty()) {
        marker.setText(InfoMarkText{"New Marker"});
    }

    m_mapData.addMarker(std::move(sharedmarker));
}

// load current element, which is expected to contain ONLY the name of an enum value
template<typename ENUM>
ENUM XmlMapStorage::loadEnum(QXmlStreamReader &stream)
{
    static_assert(std::is_enum<ENUM>::value, "template type ENUM must be an enumeration");

    const QStringRef name = stream.name();
    const QStringRef text = loadStringRef(stream);
    bool fail = false;
    const ENUM e = conv.toEnum<ENUM>(text, fail);
    if (fail) {
        throwErrorFmt(stream, "invalid <%1> content \"%2\"", name.toString(), text.toString());
    }
    return e;
}

// load current element, which is expected to contain ONLY text i.e. no attributes and no nested elements
QString XmlMapStorage::loadString(QXmlStreamReader &stream)
{
    return loadStringRef(stream).toString();
}

// load current element, which is expected to contain ONLY text i.e. no attributes and no nested elements
QStringRef XmlMapStorage::loadStringRef(QXmlStreamReader &stream)
{
    const QStringRef name = stream.name();
    if (stream.readNext() != QXmlStreamReader::Characters) {
        throwErrorFmt(stream, "invalid <%1>...</%1>", name.toString());
    }
    return stream.text();
}

QString XmlMapStorage::roomIdToString(RoomId id)
{
    return QString("%1").arg(id.asUint32());
}

void XmlMapStorage::skipXmlElement(QXmlStreamReader &stream)
{
    if (stream.tokenType() != QXmlStreamReader::EndElement) {
        stream.skipCurrentElement();
    }
}

void XmlMapStorage::throwError(QXmlStreamReader &stream, const QString &msg)
{
    QString errmsg = QString("Error at line %1:\n%2").arg(stream.lineNumber()).arg(msg);
    throw std::runtime_error(::toStdStringUtf8(errmsg));
}

// ---------------------------- XmlMapStorage::saveData() ----------------------
bool XmlMapStorage::saveData(bool baseMapOnly)
{
    log("Writing data to file ...");
    QXmlStreamWriter stream(m_file);
    saveWorld(stream, baseMapOnly);
    stream.writeEndDocument();
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
    if (e.getDoorFlags().isEmpty() && e.getExitFlags().isEmpty() && e.outIsEmpty()
        && e.getDoorName().isEmpty()) {
        return;
    }
    stream.writeStartElement("exit");
    saveXmlAttribute(stream, "dir", lowercaseDirectionC(dir));
    saveXmlAttribute(stream, "doorname", e.getDoorName().toQString());
    saveExitTo(stream, e);
    saveDoorFlags(stream, e.getDoorFlags());
    saveExitFlags(stream, e.getExitFlags());
    stream.writeEndElement(); // end exit
}

void XmlMapStorage::saveExitTo(QXmlStreamWriter &stream, const Exit &e)
{
    for (const RoomId id : e.outRange()) {
        saveXmlElement(stream, "to", roomIdToString(id));
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
    fl ^= ExitFlagEnum::EXIT; // almost always set, save it inverted
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

#undef X_FOREACH_TYPE_ENUM
