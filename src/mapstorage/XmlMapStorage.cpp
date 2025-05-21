// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "XmlMapStorage.h"

#include "../global/TextUtils.h"
#include "../global/progresscounter.h"
#include "../global/string_view_utils.h"
#include "../global/utils.h"
#include "../mainwindow/UpdateDialog.h"
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
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <QHash>
#include <QMessageBox>
#include <QString>
#include <QStringView>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

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
        return ::to_integer<T>(mmqt::as_u16string_view(str));
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
    NODISCARD QString toString(const ENUM val) const
    {
        static_assert(std::is_enum_v<ENUM>, "template type ENUM must be an enumeration");
        if constexpr (std::is_same_v<ENUM, TypeEnum>) {
#define X_CASE(x) \
    case TypeEnum::x: \
        return #x;
            //
            switch (val) {
                XFOREACH_TYPE_ENUM(X_CASE)
            }
            return "unknown";
#undef X_CASE
        } else {
            return enumToString(enumToType(val), static_cast<uint32_t>(val));
        }
    }

private:
    // define a bunch of methods
    //   static constexpr TypeEnum enumToType(RoomAlignEnum) { return TypeEnum::RoomAlignEnum; }
    //   static constexpr TypeEnum enumToType(DoorFlagEnum)  { return TypeEnum::DoorFlagEnum;  }
    //   ...
    // converting an enumeration type to its corresponding Type value,
    // which can be used as argument in enumToString() and stringToEnum()
#define X_DECL(X) \
    MAYBE_UNUSED NODISCARD static constexpr TypeEnum enumToType(X) \
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

NODISCARD static QString externalRoomIdToString(const ExternalRoomId id)
{
    // what about INVALID_EXTERNAL_ROOMID?
    return QString("%1").arg(id.asUint32());
}

NODISCARD static QString serverRoomIdToString(const ServerRoomId id)
{
    if (id == INVALID_SERVER_ROOMID) {
        return QString{};
    }

    return QString("%1").arg(id.asUint32());
}

// ---------------------------- XmlMapStorage ----------------------------------
XmlMapStorage::XmlMapStorage(const AbstractMapStorage::Data &mapData, QObject *const parent)
    : AbstractMapStorage(mapData, parent)
{}

XmlMapStorage::~XmlMapStorage() = default;

// ---------------------------- XmlMapStorage::loadData() ----------------------
std::optional<RawMapLoadData> XmlMapStorage::virt_loadData()
{
    try {
        log("Loading data ...");
        QFile &file = deref(getFile());
        QXmlStreamReader stream(&file);

        m_loading = std::make_unique<Loading>();
        m_loading->result.filename = file.fileName();
        m_loading->result.readonly = !QFileInfo(file).isWritable();
        m_loading->loadProgressDivisor = static_cast<uint64_t>(
            std::max<int64_t>(1, file.size() / LOAD_PROGRESS_MAX));

        loadWorld(stream);
        log("Finished loading.");
        return std::exchange(m_loading->result, {});

    } catch (const std::exception &ex) {
        m_loading.reset();
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        log(msg);
        qWarning().noquote() << msg;
        return std::nullopt;
    }
}

void XmlMapStorage::loadWorld(QXmlStreamReader &stream)
{
    ProgressCounter &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(LOAD_PROGRESS_MAX);

    while (stream.readNextStartElement() && !stream.hasError()) {
        if (stream.name() == "map") {
            loadMap(stream);
            break; // expecting only one <map>
        }
        skipXmlElement(stream);
    }
    if (stream.hasError()) {
        switch (stream.error()) {
        case QXmlStreamReader::NoError:
            break;
        case QXmlStreamReader::UnexpectedElementError:
            throw std::runtime_error("unexpected element");
        case QXmlStreamReader::NotWellFormedError:
            throw std::runtime_error("invalid data");
        case QXmlStreamReader::PrematureEndOfDocumentError:
            throw std::runtime_error("premature end of document");
        default:
        case QXmlStreamReader::CustomError:
            throw std::runtime_error("unknown error");
        }
    }
    loadNotifyProgress(stream);
}

// load current <map> element
void XmlMapStorage::loadMap(QXmlStreamReader &stream)
{
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
            m_loading->result.position = loadCoordinate(stream);
        } else {
            qWarning().noquote().nospace()
                << "At line " << stream.lineNumber() << ": ignoring unexpected XML element <"
                << stream.name() << "> inside <map>";
        }
        skipXmlElement(stream);
        loadNotifyProgress(stream);
    }
}

// load current <room> element
void XmlMapStorage::loadRoom(QXmlStreamReader &stream) const
{
    ExternalRawRoom room{};
    room.status = RoomStatusEnum::Permanent;

    const QXmlStreamAttributes attrs = stream.attributes();
    const QStringView externalidstr = attrs.value("id");
    const ExternalRoomId externalId = loadExternalRoomId(stream, externalidstr);
    const QStringView serveridstr = attrs.value("server_id");
    const ServerRoomId serverId = loadServerRoomId(stream, serveridstr);
    auto &loadedExternalIds = m_loading->loadedExternalRoomIds;
    if (loadedExternalIds.find(externalId) != loadedExternalIds.end()) {
        throwErrorFmt(stream, R"(duplicate room id "%1")", externalidstr.toString());
    }
    loadedExternalIds.emplace(externalId);
    if (serverId != INVALID_SERVER_ROOMID) {
        auto &loadedServerIds = m_loading->loadedServerIds;
        if (loadedServerIds.find(serverId) != loadedServerIds.end()) {
            throwErrorFmt(stream, R"(duplicate room server_id "%1")", serveridstr.toString());
        }
        loadedServerIds.emplace(serverId);
    }
    room.setId(externalId);
    room.setServerId(serverId);
    room.setName(mmqt::makeRoomName(attrs.value("name").toString()));

    auto &exitList = room.exits;
    RoomLoadFlags loadFlags;
    RoomMobFlags mobFlags;
    RoomElementEnum found = RoomElementEnum::NONE;

    while (stream.readNextStartElement() && !stream.hasError()) {
        const auto &name = stream.name();
        if (name == "area") {
            throwIfDuplicate(stream, found, RoomElementEnum::AREA);
            room.setArea(mmqt::makeRoomArea(loadString(stream)));
        } else if (name == "align") {
            throwIfDuplicate(stream, found, RoomElementEnum::ALIGN);
            room.setAlignType(loadEnum<RoomAlignEnum>(stream));
        } else if (name == "contents") {
            throwIfDuplicate(stream, found, RoomElementEnum::CONTENTS);
            room.setContents(mmqt::makeRoomContents(loadString(stream)));
        } else if (name == "coord") {
            throwIfDuplicate(stream, found, RoomElementEnum::POSITION);
            room.setPosition(loadCoordinate(stream));
        } else if (name == "description") {
            throwIfDuplicate(stream, found, RoomElementEnum::DESCRIPTION);
            room.setDescription(mmqt::makeRoomDesc(loadString(stream)));
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
            room.setNote(mmqt::makeRoomNote(loadString(stream)));
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
                << stream.name() << "> inside <room id=\"" << externalidstr << "\">";
        }
        skipXmlElement(stream);
    }
    room.setLoadFlags(loadFlags);
    room.setMobFlags(mobFlags);

    m_loading->result.rooms.emplace_back(std::move(room));
}

// convert string to RoomId
ExternalRoomId XmlMapStorage::loadExternalRoomId(QXmlStreamReader &stream, const QStringView idstr)
{
    auto fail = [&stream, idstr]() {
        throwErrorFmt(stream, R"(invalid room id "%1")", idstr.toString());
    };

    std::optional<uint32_t> id = Converter::toInteger<uint32_t>(idstr);
    if (!id.has_value()) {
        fail();
    }

    // convert number back to string, and compare the two:
    // if they differ, room ID is invalid.
    const ExternalRoomId result{id.value()};
    if (idstr != externalRoomIdToString(result)) {
        fail();
    }
    return result;
}

// convert string to RoomId
ServerRoomId XmlMapStorage::loadServerRoomId(QXmlStreamReader &stream, const QStringView idstr)
{
    // convert number back to string, and compare the two:
    // if they differ, server ID is invalid.

    if (idstr.empty()) {
        return INVALID_SERVER_ROOMID;
    }

    auto fail = [&stream, idstr] {
        throwErrorFmt(stream, R"(invalid room server_id "%1")", idstr.toString());
    };

    const std::optional<uint32_t> id = Converter::toInteger<uint32_t>(idstr);
    if (!id.has_value()) {
        fail();
    }

    ServerRoomId result{id.value()};
    if (idstr != serverRoomIdToString(result)) {
        fail();
    }
    return result;
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
void XmlMapStorage::loadExit(QXmlStreamReader &stream, ExternalRawRoom::Exits &exitList)
{
    const QXmlStreamAttributes attrs = stream.attributes();
    const ExitDirEnum dir = directionForLowercase(attrs.value("dir"));
    DoorFlags doorFlags;
    ExitFlags exitFlags;
    ExternalRawExit &exit = exitList[dir];
    exit.setDoorName(DoorName{attrs.value("doorname").toString()});

    while (stream.readNextStartElement() && !stream.hasError()) {
        const auto &name = stream.name();
        if (name == "to") {
            exit.outgoing.insert(loadExternalRoomId(stream, loadStringView(stream)));
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

void XmlMapStorage::loadMarker(QXmlStreamReader &stream) const
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

    InfoMarkFields marker{};
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
                marker.setText(mmqt::makeInfoMarkText(loadString(stream)));
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
        marker.setText(mmqt::makeInfoMarkText("New Marker"));
    }

    if (!m_loading->result.markerData) {
        m_loading->result.markerData.emplace();
    }
    RawMarkerData &data = *m_loading->result.markerData;
    data.markers.emplace_back(std::move(marker));
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

void XmlMapStorage::skipXmlElement(QXmlStreamReader &stream)
{
    if (stream.tokenType() != QXmlStreamReader::EndElement) {
        stream.skipCurrentElement();
    }
    if (stream.tokenType() != QXmlStreamReader::EndElement) {
        throw std::runtime_error("missing end tag");
    }
}

void XmlMapStorage::loadNotifyProgress(QXmlStreamReader &stream)
{
    auto &loading = *m_loading;
    const auto loadProgressNew = static_cast<uint32_t>(
        static_cast<uint64_t>(stream.characterOffset()) / loading.loadProgressDivisor);
    if (loadProgressNew <= loading.loadProgress) {
        return;
    }
    getProgressCounter().step(loadProgressNew - loading.loadProgress);
    loading.loadProgress = loadProgressNew;
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
bool XmlMapStorage::virt_saveData(const RawMapData &map)
{
    m_saving = std::make_unique<Saving>(map);
    try {
        log("Writing data to file ...");
        QXmlStreamWriter stream(getFile());
        saveWorld(stream);
        stream.writeEndDocument();
        log("Writing data finished.");
        return true;
    } catch (...) {
        m_saving.reset();
        throw;
    }
}

void XmlMapStorage::saveWorld(QXmlStreamWriter &stream)
{
    // Collect the room and marker lists. The room list can't be acquired
    // directly apparently and we have to go through a RoomSaver which receives
    // them from a sort of callback function.
    // The RoomSaver acts as a lock on the rooms.
    auto &map = m_saving->map.mapPair.modified;
    const auto trueRoomCount = map.getRoomsCount();
    const auto roomsCount = static_cast<uint32_t>(trueRoomCount);
    if (trueRoomCount != roomsCount) {
        throw std::runtime_error("too many rooms");
    }

    const auto &opt_markers = m_saving->map.markerData;
    const auto markerCount = static_cast<uint32_t>(opt_markers ? opt_markers->size() : 0u);

    ProgressCounter &progressCounter = getProgressCounter();
    progressCounter.reset();
    progressCounter.increaseTotalStepsBy(roomsCount + markerCount);

    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("map");
    saveXmlAttribute(stream, "type", "mmapper2xml");
    saveXmlAttribute(stream, "version", "1.0.0");

    progressCounter.setCurrentTask(ProgressMsg{"Saving rooms..."});
    saveRooms(stream, map.getRooms());
    if (opt_markers && !opt_markers->empty()) {
        progressCounter.setCurrentTask(ProgressMsg{"Saving markers..."});
        saveMarkers(stream, *opt_markers);
    }
    // write selected room x,y,z
    saveCoordinate(stream, "position", m_saving->map.position);

    stream.writeEndElement(); // end map
}

void XmlMapStorage::saveRooms(QXmlStreamWriter &stream, const RoomIdSet &roomList)
{
    const Map &map = m_saving->map.mapPair.modified;
    ProgressCounter &progressCounter = getProgressCounter();

    for (const RoomId id : roomList) {
        if (auto handle = map.getRoomHandle(id)) {
            saveRoom(stream, handle.getRawCopyExternal());
        }
        progressCounter.step();
    }
}

void XmlMapStorage::saveRoom(QXmlStreamWriter &stream, const ExternalRawRoom &room)
{
    stream.writeStartElement("room");

    const ExternalRoomId externalId = room.getId();
    saveXmlAttribute(stream, "id", externalRoomIdToString(externalId));
    const ServerRoomId serverId = room.getServerId();
    saveXmlAttribute(stream, "server_id", serverRoomIdToString(serverId));
    saveXmlAttribute(stream, "name", room.getName().toQString());
    saveXmlElement(stream, "area", room.getArea().toQString());
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
        // REVISIT: if (room.hasExit(dir)) ?
        saveExit(stream, room.getExit(dir), dir);
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

void XmlMapStorage::saveExit(QXmlStreamWriter &stream,
                             const ExternalRawExit &e,
                             const ExitDirEnum dir)
{
    // was !e.exitIsExit() || e.outIsEmpty()
    // now it saves a TON of extra (NO_EXIT | NO_MATCH) exits
    // is that a bug or a misguided feature?
    if (e.getDoorFlags().isEmpty() && e.getExitFlags().isEmpty() && e.outIsEmpty()
        && e.getDoorName().isEmpty()) {
        return;
    }
    // TODO: RAII writeStartElement() + writeEndElement()
    stream.writeStartElement("exit");
    saveXmlAttribute(stream, "dir", lowercaseDirection(dir));
    saveXmlAttribute(stream, "doorname", e.getDoorName().toQString());
    saveExitTo(stream, e);
    saveDoorFlags(stream, e.getDoorFlags());
    saveExitFlags(stream, e.getExitFlags());
    stream.writeEndElement(); // end exit
}

void XmlMapStorage::saveExitTo(QXmlStreamWriter &stream, const ExternalRawExit &e)
{
    for (const ExternalRoomId id : e.outgoing) {
        saveXmlElement(stream, "to", externalRoomIdToString(id));
    }
}

void XmlMapStorage::saveMarkers(QXmlStreamWriter &stream, const RawMarkerData &markerList)
{
    if (markerList.empty()) {
        return;
    }

    ProgressCounter &progressCounter = getProgressCounter();
    for (const auto &marker : markerList.markers) {
        saveMarker(stream, marker);
        progressCounter.step();
    }
}

void XmlMapStorage::saveMarker(QXmlStreamWriter &stream, const InfoMarkFields &marker)
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
