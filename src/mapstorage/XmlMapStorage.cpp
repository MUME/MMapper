// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

#include "../global/string_view_utils.h"
#include "../mainwindow/UpdateDialog.h"
#include "../map/enums.h"
#include "abstractmapstorage.h"
#include "basemapsavefilter.h"
#include "roomsaver.h"

#include <stdexcept>
#include <type_traits>
#include <vector>

#include <QHash>
#include <QMessageBox>
#include <QString>
#include <QStringView>
#include <QXmlStreamReader>

// ---------------------------- XmlMapStorage::TypeEnum ------------------------
// list know enum types
#define XFOREACH_TYPE_ENUM(X) \
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
    X(TypeEnum)

namespace { // anonymous

enum class NODISCARD TypeEnum : uint32_t {
#define X_DECL(X) X,
    XFOREACH_TYPE_ENUM(X_DECL)
#undef X_DECL
};

#define X_ADD(X) +1 // NOLINT
constexpr const size_t NUM_XMLMAPSTORAGE_TYPE = (XFOREACH_TYPE_ENUM(X_ADD));
#undef X_ADD

// ---------------------------- XmlMapStorage::Converter -----------------------
class NODISCARD Converter final
{
private:
    std::vector<std::vector<QString>> m_enumToStrings;
    std::vector<QHash<QStringView, uint32_t>> m_stringToEnums;

public:
    Converter();
    ~Converter() = default;

    // parse string containing a signed or unsigned number.
    template<typename T>
    NODISCARD static std::optional<T> toInteger(const QStringView str)
    {
        return ::to_integer<T>(as_u16string_view(str));
    }

    // parse string containing the name of an enum.
    template<typename ENUM>
    NODISCARD std::optional<ENUM> toEnum(const QStringView str) const
    {
        static_assert(std::is_enum_v<ENUM>, "template type ENUM must be an enumeration");
        if (const auto opt = stringToEnum(enumToType(static_cast<ENUM>(0)), str); opt.has_value()) {
            return static_cast<ENUM>(opt.value());
        }
        return std::nullopt;
    }

    template<typename ENUM>
    NODISCARD const QString &toString(const ENUM val) const
    {
        static_assert(std::is_enum_v<ENUM>, "template type ENUM must be an enumeration");
        return enumToString(enumToType(val), static_cast<uint32_t>(val));
    }

private:
    // define a bunch of methods
    //   static constexpr TypeEnum enumToType(RoomAlignEnum) { return TypeEnum::RoomAlignEnum; }
    //   static constexpr TypeEnum enumToType(DoorFlagEnum)  { return TypeEnum::DoorFlagEnum;  }
    //   ...
    // converting an enumeration type to its corresponding Type value,
    // which can be used as argument in enumToString() and stringToEnum()
#define X_DECL(X) \
    NODISCARD static constexpr TypeEnum enumToType(X) \
    { \
        return TypeEnum::X; \
    }
    XFOREACH_TYPE_ENUM(X_DECL)
#undef X_DECL

    NODISCARD std::optional<uint32_t> stringToEnum(TypeEnum type, QStringView str) const;
    NODISCARD const QString &enumToString(TypeEnum type, uint32_t val) const;
};

Converter::Converter()
    : m_enumToStrings{
#define X_DECL(X) /* */ {#X},
#define X_DECL2(X, ...) {#X},
        /* these must match the enum types listed in XFOREACH_TYPE_ENUM above */
        {XFOREACH_RoomAlignEnum(X_DECL)},
        {XFOREACH_DOOR_FLAG(X_DECL2)},
        {XFOREACH_EXIT_FLAG(X_DECL2)},
        {XFOREACH_RoomLightEnum(X_DECL)},
        {XFOREACH_ROOM_LOAD_FLAG(X_DECL)},
        {XFOREACH_INFOMARK_CLASS(X_DECL)},
        {XFOREACH_INFOMARK_TYPE(X_DECL)},
        {XFOREACH_ROOM_MOB_FLAG(X_DECL)},
        {XFOREACH_RoomPortableEnum(X_DECL)},
        {XFOREACH_RoomRidableEnum(X_DECL)},
        {XFOREACH_RoomSundeathEnum(X_DECL)},
        {XFOREACH_RoomTerrainEnum(X_DECL)},
        {XFOREACH_TYPE_ENUM(X_DECL)},
#undef X_DECL
#undef X_DECL2
    }
{
    if (m_enumToStrings.size() != NUM_XMLMAPSTORAGE_TYPE) {
        throw std::runtime_error("XmlMapStorage internal error: enum names do not match enum types");
    }

    // create the maps string -> enum value for each enum type listed above
    for (auto &vec : m_enumToStrings) {
        auto &map = m_stringToEnums.emplace_back();
        uint32_t val = 0;
        for (auto &str : vec) {
            if (str == "UNDEFINED") {
                str.clear(); // we never save or load the string "UNDEFINED"
            } else {
                if (str == "EXIT") {
                    // we save the EXIT flag inverted => invert the name too
                    str = "NO_EXIT";
                }
                map[str] = val;
            }
            ++val;
        }
    }
}

const QString &Converter::enumToString(const TypeEnum type, const uint32_t val) const
{
    const auto index = static_cast<uint32_t>(type);
    if (index < m_enumToStrings.size()) {
        const auto &tmp = m_enumToStrings[index];
        if (val < tmp.size()) {
            return tmp[val];
        }
    }

    qWarning().noquote().nospace()
        << "Attempt to save an invalid enum type = " << toString(type) << ", value = " << val
        << ". Either the current map is damaged, or there is a bug";

    static const QString g_empty{};
    assert(g_empty.isEmpty());
    return g_empty;
}

std::optional<uint32_t> Converter::stringToEnum(const TypeEnum type, const QStringView str) const
{
    const auto index = static_cast<uint32_t>(type);
    if (index < m_stringToEnums.size()) {
        const auto &tmp = m_stringToEnums[index];
        const auto iter = tmp.find(str);
        if (iter != tmp.end()) {
            return iter.value();
        }
    }
    return std::nullopt;
}

const Converter conv;

NODISCARD ExitDirEnum directionForLowercase(const QStringRef &lowcase)
{
    if (lowcase.isEmpty()) {
        return ExitDirEnum::UNKNOWN;
    }

    switch (lowcase.front().unicode()) {
    case 'n':
        if (lowcase == "north") {
            return ExitDirEnum::NORTH;
        }
        break;
    case 's':
        if (lowcase == "south") {
            return ExitDirEnum::SOUTH;
        }
        break;
    case 'e':
        if (lowcase == "east") {
            return ExitDirEnum::EAST;
        }
        break;
    case 'w':
        if (lowcase == "west") {
            return ExitDirEnum::WEST;
        }
        break;
    case 'u':
        if (lowcase == "up") {
            return ExitDirEnum::UP;
        }
        break;
    case 'd':
        if (lowcase == "down") {
            return ExitDirEnum::DOWN;
        }
        break;
    default:
        break;
    }

    return ExitDirEnum::UNKNOWN;
}

} // namespace

// ---------------------------- XmlMapStorage ----------------------------------
XmlMapStorage::XmlMapStorage(MapData &mapdata,
                             const QString &filename,
                             QFile *const file,
                             QObject *const parent)
    : AbstractMapStorage(mapdata, filename, file, parent)
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
        if (stream.name() == "map") {
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
                          R"(unsupported map type="%1",)"
                          "\n"
                          R"(expecting type="mmapper2xml")",
                          type);
        }
        const QString version = attrs.value("version").toString();
        const CompareVersion cmp(version);
        if (cmp.major() != 1) {
            throwErrorFmt(stream,
                          R"(unsupported map version="%1",)"
                          "\n"
                          R"(expecting version="1.x.y")",
                          version);
        }
    }

    while (stream.readNextStartElement() && !stream.hasError()) {
        const auto &name = stream.name();
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
        const auto &name = stream.name();
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

    m_loadedRooms.emplace(roomId, sharedroom);
}

// convert string to RoomId
RoomId XmlMapStorage::loadRoomId(QXmlStreamReader &stream, const QStringView idstr)
{
    const std::optional<RoomId> id{Converter::toInteger<uint32_t>(idstr)};
    // convert number back to string, and compare the two:
    // if they differ, room ID is invalid.
    if (!id.has_value() || idstr != roomIdToString(id.value())) {
        throwErrorFmt(stream, "invalid room id \"%1\"", idstr.toString());
    }
    return id.value();
}

// load current <coord> element
Coordinate XmlMapStorage::loadCoordinate(QXmlStreamReader &stream)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    const auto x = Converter::toInteger<int>(attrs.value("x"));
    const auto y = Converter::toInteger<int>(attrs.value("y"));
    const auto z = Converter::toInteger<int>(attrs.value("z"));
    if (!x.has_value() || !y.has_value() || !z.has_value()) {
        throwErrorFmt(stream,
                      R"(invalid coordinate values x="%1" y="%2" z="%3")",
                      attrs.value("x").toString(),
                      attrs.value("y").toString(),
                      attrs.value("z").toString());
    }
    return Coordinate(x.value(), y.value(), z.value());
}

// load current <exit> element
void XmlMapStorage::loadExit(QXmlStreamReader &stream, ExitsList &exitList)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    const ExitDirEnum dir = directionForLowercase(attrs.value("dir"));
    DoorFlags doorFlags;
    ExitFlags exitFlags;
    Exit &exit = exitList[dir];
    exit.setDoorName(DoorName{attrs.value("doorname").toString()});

    while (stream.readNextStartElement() && !stream.hasError()) {
        const auto &name = stream.name();
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
    const auto type = conv.toEnum<InfoMarkTypeEnum>(attrs.value("type"));
    const auto clas = conv.toEnum<InfoMarkClassEnum>(attrs.value("class"));
    if (!type.has_value() || !clas.has_value()) {
        throwErrorFmt(stream,
                      R"(invalid marker attributes type="%1" class="%2")",
                      attrs.value("type").toString(),
                      attrs.value("class").toString());
    }
    QStringView anglestr = attrs.value("angle");
    int angle = 0;
    if (!anglestr.isNull()) {
        const auto opt_angle = Converter::toInteger<int>(anglestr);
        if (!opt_angle.has_value()) {
            throwErrorFmt(stream, R"(invalid marker attribute angle="%1")", anglestr.toString());
        }
        angle = opt_angle.value();
    }

    SharedInfoMark sharedmarker = InfoMark::alloc(m_mapData);
    InfoMark &marker = deref(sharedmarker);
    size_t foundPos1 = 0;
    size_t foundPos2 = 0;

    marker.setType(type.value());
    marker.setClass(clas.value());
    marker.setRotationAngle(angle);

    while (stream.readNextStartElement() && !stream.hasError()) {
        const auto &name = stream.name();
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
                   R"(invalid marker: missing mandatory element <pos1 x="..." y="..." z="..."/>)");
    } else if (foundPos1 > 1) {
        throwError(stream, R"(invalid marker: duplicate element <pos1 x="..." y="..." z="..."/>)");
    }

    if (foundPos2 == 0) {
        // saveMarker() omits pos2 when it's equal to pos1
        marker.setPosition2(marker.getPosition1());
    } else if (foundPos2 > 1) {
        throwError(stream, R"(invalid marker: duplicate element <pos2 x="..." y="..." z="..."/>)");
    }

    // REVISIT: Just discard empty text markers?
    if (type == InfoMarkTypeEnum::TEXT && marker.getText().isEmpty()) {
        marker.setText(InfoMarkText{"New Marker"});
    }

    m_mapData.addMarker(sharedmarker);
}

// load current element, which is expected to contain ONLY the name of an enum value
template<typename ENUM>
ENUM XmlMapStorage::loadEnum(QXmlStreamReader &stream)
{
    static_assert(std::is_enum_v<ENUM>, "template type ENUM must be an enumeration");

    // copy name in case it's somehow modified by loadStringView()
    const auto name = stream.name();
    const QStringView text = loadStringView(stream);
    const std::optional<ENUM> e = conv.toEnum<ENUM>(text);
    if (!e.has_value()) {
        throwErrorFmt(stream, R"(invalid <%1> content "%2")", name.toString(), text.toString());
    }
    return e.value();
}

// load current element, which is expected to contain ONLY text i.e. no attributes and no nested elements
QString XmlMapStorage::loadString(QXmlStreamReader &stream)
{
    return loadStringView(stream).toString();
}

// load current element, which is expected to contain ONLY text i.e. no attributes and no nested elements
QStringView XmlMapStorage::loadStringView(QXmlStreamReader &stream)
{
    // copy name in case it's somehow modified by readNext()
    const auto name = stream.name();
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
    auto loadProgressNew = static_cast<uint32_t>(static_cast<uint64_t>(stream.characterOffset())
                                                 / m_loadProgressDivisor);
    if (loadProgressNew <= m_loadProgress) {
        return;
    }
    getProgressCounter().step(loadProgressNew - m_loadProgress);
    m_loadProgress = loadProgressNew;
}

void XmlMapStorage::throwError(QXmlStreamReader &stream, const QString &msg)
{
    QString errmsg = QString("Error at line %1:\n%2").arg(stream.lineNumber()).arg(msg);
    throw std::runtime_error(mmqt::toStdStringUtf8(errmsg));
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

#undef XFOREACH_TYPE_ENUM
