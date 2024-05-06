// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

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

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

#include <QHash>
#include <QMessageBox>
#include <QString>
#include <QStringView>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

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

#define ADD(X) +1
static constexpr const uint NUM_XMLMAPSTORAGE_TYPE = (X_FOREACH_TYPE_ENUM(ADD));
#undef ADD

// ---------------------------- XmlMapStorage::Converter -----------------------
class XmlMapStorage::Converter final
{
public:
    Converter();
    ~Converter() = default;

    // parse string containing a signed or unsigned number.
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename T>
    static T toInteger(const QStringView str, bool &fail)
    {
        bool ok = false;
        const T ret = to_integer<T>(as_u16string_view(str), ok);
        if (!ok) {
            fail = true;
        }
        return ret;
    }

    // parse string containing the name of an enum.
    // sets fail = true only in case of errors, otherwise fail is not modified
    template<typename ENUM>
    ENUM toEnum(const QStringView str, bool &fail) const
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
    static constexpr Type enumToType(X) \
    { \
        return Type::X; \
    }
    X_FOREACH_TYPE_ENUM(DECL)
#undef DECL

    uint stringToEnum(Type type, const QStringView str, bool &fail) const;
    const QString &enumToString(Type type, uint val) const;

    std::vector<std::vector<QString>> enumToStrings;
    std::vector<QHash<QStringView, uint>> stringToEnums;
    const QString empty;
};

XmlMapStorage::Converter::Converter()
    : enumToStrings{
#define DECL(X) /* */ {#X},
#define DECL_(X, ...) {#X},
        /* these must match the enum types listed in X_FOREACH_TYPE_ENUM above */
        {X_FOREACH_RoomAlignEnum(DECL)},
        {X_FOREACH_DOOR_FLAG(DECL_)},
        {X_FOREACH_EXIT_FLAG(DECL_)},
        {X_FOREACH_RoomLightEnum(DECL)},
        {X_FOREACH_ROOM_LOAD_FLAG(DECL)},
        {X_FOREACH_INFOMARK_CLASS(DECL)},
        {X_FOREACH_INFOMARK_TYPE(DECL)},
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
    if (enumToStrings.size() != NUM_XMLMAPSTORAGE_TYPE) {
        throw std::runtime_error("XmlMapStorage internal error: enum names do not match enum types");
    }

    // create the maps string -> enum value for each enum type listed above
    for (std::vector<QString> &vec : enumToStrings) {
        stringToEnums.emplace_back();
        QHash<QStringView, uint> &map = stringToEnums.back();
        uint val = 0;
        for (QString &str : vec) {
            if (str == "UNDEFINED") {
                str.clear(); // we never save or load the string "UNDEFINED"
            } else {
                if (str == "EXIT") {
                    // we save the EXIT flag inverted => invert the name too
                    str = "NO_EXIT";
                }
                map[QStringView(str)] = val;
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

uint XmlMapStorage::Converter::stringToEnum(Type type, const QStringView str, bool &fail) const
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
                             QObject *const parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
    , m_loadedRooms()
    , m_loadProgressDivisor(1) // avoid division by zero
    , m_loadProgress(0)
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
        m_loadProgressDivisor = std::max<uint64_t> //
            (1, static_cast<uint64_t>(m_file->size()) / LOAD_PROGRESS_MAX);
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
    progressCounter.increaseTotalStepsBy(LOAD_PROGRESS_MAX);

    MapFrontendBlocker blocker(m_mapData);
    m_mapData.setDataChanged();

    while (stream.readNextStartElement() && !stream.hasError()) {
        if (as_u16string_view(stream.name()) == "map") {
            loadMap(stream);
            break; // expecting only one <map>
        }
        skipXmlElement(stream);
    }
    loadNotifyProgress(stream);
}

// load current <map> element
void XmlMapStorage::loadMap(QXmlStreamReader &stream)
{
    m_loadedRooms.clear();
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

    while (stream.readNextStartElement() && !stream.hasError()) {
        const std::u16string_view name = as_u16string_view(stream.name());
        if (name == "room") {
            loadRoom(stream);
        } else if (name == "marker") {
            loadMarker(stream);
        } else if (name == "position") {
            m_mapData.setPosition(loadCoordinate(stream));
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << stream.name() << "> inside <map>";
        }
        skipXmlElement(stream);
        loadNotifyProgress(stream);
    }
    connectRoomsExitFrom(stream);
    moveRoomsToMapData();
}

// load current <room> element
void XmlMapStorage::loadRoom(QXmlStreamReader &stream)
{
    const SharedRoom sharedroom = Room::createPermanentRoom(m_mapData);
    Room &room = deref(sharedroom);

    const QXmlStreamAttributes attrs = stream.attributes();
    const QStringView idstr = attrs.value("id");
    const RoomId roomId = loadRoomId(stream, idstr);
    if (m_loadedRooms.count(roomId) != 0) {
        throwErrorFmt(stream, "duplicate room id \"%1\"", idstr.toString());
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
    RoomElementEnum found = RoomElementEnum::NONE;

    while (stream.readNextStartElement() && !stream.hasError()) {
        const std::u16string_view name = as_u16string_view(stream.name());
        if (name == "align") {
            throwIfDuplicate(stream, found, RoomElementEnum::ALIGN);
            room.setAlignType(loadEnum<RoomAlignEnum>(stream));
        } else if (name == "contents") {
            throwIfDuplicate(stream, found, RoomElementEnum::CONTENTS);
            room.setContents(RoomContents{loadString(stream)});
        } else if (name == "coord") {
            throwIfDuplicate(stream, found, RoomElementEnum::POSITION);
            room.setPosition(loadCoordinate(stream));
        } else if (name == "description") {
            throwIfDuplicate(stream, found, RoomElementEnum::DESCRIPTION);
            room.setDescription(RoomDesc{loadString(stream)});
        } else if (name == "exit") {
            loadExit(stream, exitList);
        } else if (name == "light") {
            throwIfDuplicate(stream, found, RoomElementEnum::LIGHT);
            room.setLightType(loadEnum<RoomLightEnum>(stream));
        } else if (name == "loadflag") {
            loadFlags |= loadEnum<RoomLoadFlagEnum>(stream);
        } else if (name == "mobflag") {
            mobFlags |= loadEnum<RoomMobFlagEnum>(stream);
        } else if (name == "note") {
            throwIfDuplicate(stream, found, RoomElementEnum::NOTE);
            room.setNote(RoomNote{loadString(stream)});
        } else if (name == "portable") {
            throwIfDuplicate(stream, found, RoomElementEnum::PORTABLE);
            room.setPortableType(loadEnum<RoomPortableEnum>(stream));
        } else if (name == "ridable") {
            throwIfDuplicate(stream, found, RoomElementEnum::RIDABLE);
            room.setRidableType(loadEnum<RoomRidableEnum>(stream));
        } else if (name == "sundeath") {
            throwIfDuplicate(stream, found, RoomElementEnum::SUNDEATH);
            room.setSundeathType(loadEnum<RoomSundeathEnum>(stream));
        } else if (name == "terrain") {
            throwIfDuplicate(stream, found, RoomElementEnum::TERRAIN);
            room.setTerrainType(loadEnum<RoomTerrainEnum>(stream));
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << stream.name() << "> inside <room id=\"" << idstr << "\">";
        }
        skipXmlElement(stream);
    }
    room.setExitsList(exitList);
    room.setLoadFlags(loadFlags);
    room.setMobFlags(mobFlags);

    m_loadedRooms.emplace(std::move(roomId), std::move(sharedroom));
}

// convert string to RoomId
RoomId XmlMapStorage::loadRoomId(QXmlStreamReader &stream, const QStringView idstr)
{
    bool fail = false;
    const RoomId id{conv.toInteger<uint32_t>(idstr, fail)};
    // convert number back to string, and compare the two:
    // if they differ, room ID is invalid.
    if (fail || idstr != roomIdToString(id)) {
        throwErrorFmt(stream, "invalid room id \"%1\"", idstr.toString());
    }
    return id;
}

// load current <coord> element
Coordinate XmlMapStorage::loadCoordinate(QXmlStreamReader &stream)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    bool fail = false;
    const int x = conv.toInteger<int>(attrs.value("x"), fail);
    const int y = conv.toInteger<int>(attrs.value("y"), fail);
    const int z = conv.toInteger<int>(attrs.value("z"), fail);
    if (fail) {
        throwErrorFmt(stream,
                      "invalid coordinate values x=\"%1\" y=\"%2\" z=\"%3\"",
                      attrs.value("x").toString(),
                      attrs.value("y").toString(),
                      attrs.value("z").toString());
    }
    return Coordinate(x, y, z);
}

static ExitDirEnum directionForLowercase(const std::u16string_view lowcase)
{
    if (lowcase.empty())
        return ExitDirEnum::UNKNOWN;

    switch (lowcase[0]) {
    case 'n':
        if (lowcase == "north")
            return ExitDirEnum::NORTH;
        break;
    case 's':
        if (lowcase == "south")
            return ExitDirEnum::SOUTH;
        break;
    case 'e':
        if (lowcase == "east")
            return ExitDirEnum::EAST;
        break;
    case 'w':
        if (lowcase == "west")
            return ExitDirEnum::WEST;
        break;
    case 'u':
        if (lowcase == "up")
            return ExitDirEnum::UP;
        break;
    case 'd':
        if (lowcase == "down")
            return ExitDirEnum::DOWN;
        break;
    default:
        break;
    }

    return ExitDirEnum::UNKNOWN;
}

// load current <exit> element
void XmlMapStorage::loadExit(QXmlStreamReader &stream, ExitsList &exitList)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    const ExitDirEnum dir = directionForLowercase(as_u16string_view(attrs.value("dir")));
    DoorFlags doorFlags;
    ExitFlags exitFlags;
    Exit &exit = exitList[dir];
    exit.setDoorName(DoorName{attrs.value("doorname").toString()});

    while (stream.readNextStartElement() && !stream.hasError()) {
        const std::u16string_view name = as_u16string_view(stream.name());
        if (name == "to") {
            exit.addOut(loadRoomId(stream, loadStringView(stream)));
        } else if (name == "doorflag") {
            doorFlags |= loadEnum<DoorFlagEnum>(stream);
        } else if (name == "exitflag") {
            exitFlags |= loadEnum<ExitFlagEnum>(stream);
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << stream.name() << "> inside <exit>";
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
    for (auto &elem : m_loadedRooms) {
        const Room &room = deref(elem.second);
        for (const ExitDirEnum dir : ALL_EXITS7) {
            connectRoomExitFrom(stream, room, dir);
        }
    }
}

void XmlMapStorage::connectRoomExitFrom(QXmlStreamReader &stream,
                                        const Room &fromRoom,
                                        const ExitDirEnum dir)
{
    const Exit &fromE = fromRoom.exit(dir);
    if (fromE.outIsEmpty()) {
        return;
    }
    const RoomId fromId = fromRoom.getId();
    for (const RoomId toId : fromE.outRange()) {
        const auto iter = m_loadedRooms.find(toId);
        if (iter == m_loadedRooms.end()) {
            throwErrorFmt(stream,
                          "room %1 has exit %2 to non-existing room %3",
                          roomIdToString(fromId),
                          lowercaseDirection(dir),
                          roomIdToString(toId));
        }
        Room &toRoom = deref(iter->second);
        toRoom.addInExit(opposite(dir), fromId);
    }
}

// add all loaded rooms to m_mapData
void XmlMapStorage::moveRoomsToMapData()
{
    for (const auto &elem : m_loadedRooms) {
        m_mapData.insertPredefinedRoom(elem.second);
    }
    m_loadedRooms.clear();
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
    QStringView anglestr = attrs.value("angle");
    int angle = 0;
    if (!anglestr.isNull()) {
        angle = conv.toInteger<int>(anglestr, fail);
        if (fail) {
            throwErrorFmt(stream, "invalid marker attribute angle=\"%1\"", anglestr.toString());
        }
    }

    SharedInfoMark sharedmarker = InfoMark::alloc(m_mapData);
    InfoMark &marker = deref(sharedmarker);
    size_t foundPos1 = 0;
    size_t foundPos2 = 0;

    marker.setType(type);
    marker.setClass(clas);
    marker.setRotationAngle(angle);

    while (stream.readNextStartElement() && !stream.hasError()) {
        const std::u16string_view name = as_u16string_view(stream.name());
        if (name == "pos1") {
            marker.setPosition1(loadCoordinate(stream));
            ++foundPos1;
        } else if (name == "pos2") {
            marker.setPosition2(loadCoordinate(stream));
            ++foundPos2;
        } else if (name == "text") {
            // load text only if type == TEXT
            if (type == InfoMarkTypeEnum::TEXT) {
                marker.setText(InfoMarkText{loadString(stream)});
            }
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << stream.name() << "> inside <marker>";
        }
        skipXmlElement(stream);
    }

    if (foundPos1 == 0) {
        throwError(stream,
                   "invalid marker: missing mandatory element <pos1 x=\"...\" y=\"...\" z=\"...\"/>");
    } else if (foundPos1 > 1) {
        throwError(stream,
                   "invalid marker: duplicate element <pos1 x=\"...\" y=\"...\" z=\"...\"/>");
    }

    if (foundPos2 == 0) {
        // saveMarker() omits pos2 when it's equal to pos1
        marker.setPosition2(marker.getPosition1());
    } else if (foundPos2 > 1) {
        throwError(stream,
                   "invalid marker: duplicate element <pos2 x=\"...\" y=\"...\" z=\"...\"/>");
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

    const QStringView name = stream.name();
    const QStringView text = loadStringView(stream);
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
    return loadStringView(stream).toString();
}

// load current element, which is expected to contain ONLY text i.e. no attributes and no nested elements
QStringView XmlMapStorage::loadStringView(QXmlStreamReader &stream)
{
    const QStringView name = stream.name();
    if (stream.readNext() != QXmlStreamReader::Characters) {
        throwErrorFmt(stream, "invalid <%1>...</%1>", name.toString());
    }
    return stream.text();
}

QString XmlMapStorage::roomIdToString(const RoomId id)
{
    return QString("%1").arg(id.asUint32());
}

void XmlMapStorage::skipXmlElement(QXmlStreamReader &stream)
{
    if (stream.tokenType() != QXmlStreamReader::EndElement) {
        stream.skipCurrentElement();
    }
}

void XmlMapStorage::loadNotifyProgress(QXmlStreamReader &stream)
{
    uint32_t loadProgressNew = static_cast<uint32_t> //
        (static_cast<uint64_t>(stream.characterOffset()) / m_loadProgressDivisor);
    if (loadProgressNew <= m_loadProgress) {
        return;
    }
    getProgressCounter().step(loadProgressNew - m_loadProgress);
    m_loadProgress = loadProgressNew;
}

void XmlMapStorage::throwError(QXmlStreamReader &stream, const QString &msg)
{
    QString errmsg = QString("Error at line %1:\n%2").arg(stream.lineNumber()).arg(msg);
    throw std::runtime_error(::toStdStringUtf8(errmsg));
}

void XmlMapStorage::throwIfDuplicate(QXmlStreamReader &stream,
                                     RoomElementEnum &set,
                                     RoomElementEnum curr)
{
    if ((uint32_t(set) & uint32_t(curr)) != 0) {
        throwErrorFmt(stream, "invalid room: duplicate element <%1>", stream.name().toString());
    }
    set = RoomElementEnum(uint32_t(set) | uint32_t(curr));
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
    const uint32_t roomsCount = m_mapData.getRoomsCount();

    ConstRoomList roomList;
    roomList.reserve(roomsCount);

    RoomSaver saver(m_mapData, roomList);
    for (uint32_t i = 0; i < roomsCount; ++i) {
        m_mapData.lookingForRooms(saver, RoomId{i});
    }
    const MarkerList &markerList = m_mapData.getMarkersList();

    ProgressCounter &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(saver.getRoomsCount()
                                         + static_cast<uint32_t>(markerList.size()));

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

void XmlMapStorage::saveExit(QXmlStreamWriter &stream, const Exit &e, const ExitDirEnum dir)
{
    if (e.getDoorFlags().isEmpty() && e.getExitFlags().isEmpty() && e.outIsEmpty()
        && e.getDoorName().isEmpty()) {
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
                         QString("%1").arg(static_cast<int32_t>(marker.getRotationAngle())));
    }
    saveCoordinate(stream, "pos1", marker.getPosition1());
    if (marker.getPosition1() != marker.getPosition2()) {
        saveCoordinate(stream, "pos2", marker.getPosition2());
    }

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

void XmlMapStorage::saveDoorFlags(QXmlStreamWriter &stream, const DoorFlags fl)
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

void XmlMapStorage::saveExitFlags(QXmlStreamWriter &stream, const ExitFlags fl)
{
    ExitFlags copy = fl;
    copy ^= ExitFlagEnum::EXIT; // almost always set, save it inverted
    if (copy.isEmpty()) {
        return;
    }
    for (const ExitFlagEnum e : ALL_EXIT_FLAGS) {
        if (copy.contains(e)) {
            saveXmlElement(stream, "exitflag", conv.toString(e));
        }
    }
}

void XmlMapStorage::saveRoomLoadFlags(QXmlStreamWriter &stream, const RoomLoadFlags fl)
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

void XmlMapStorage::saveRoomMobFlags(QXmlStreamWriter &stream, const RoomMobFlags fl)
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
