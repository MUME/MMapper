// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/CaseUtils.h"
#include "../group/mmapper2group.h"
#include "../map/ExitsFlags.h"
#include "../map/ParseTree.h"
#include "../map/PromptFlags.h"
#include "../map/RawRoom.h"
#include "../map/parseevent.h"
#include "../mapdata/mapdata.h"
#include "../proxy/GmcpMessage.h"
#include "abstractparser.h"
#include "mumexmlparser.h"

#include <optional>
#include <utility>

#include <QByteArray>
#include <QString>

namespace { // anonymous
static volatile bool verbose_debugging = IS_DEBUG_BUILD;
} // namespace

void MumeXmlParser::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (!msg.getJsonDocument().has_value()) {
        return;
    }

    auto pObj = msg.getJsonDocument()->getObject();
    if (!pObj) {
        return;
    }
    const auto &obj = *pObj;

    if (msg.isCharStatusVars()) {
        parseGmcpStatusVars(obj);
    } else if (msg.isCharVitals()) {
        parseGmcpCharVitals(obj);
    } else if (msg.isEventMoved()) {
        parseGmcpEventMoved(obj);
    } else if (msg.isRoomInfo()) {
        parseGmcpRoomInfo(obj);
    }
}

void MumeXmlParser::parseGmcpStatusVars(const JsonObj &obj)
{
    // "Char.StatusVars {\"race\":\"Troll\",\"subrace\":\"Cave Troll\"}"
    if (auto race = obj.getString("race")) {
        auto &trollExitMapping = m_commonData.trollExitMapping;
        trollExitMapping = areEqualAsLowerUtf8(race->toUtf8().toStdString(), "Troll");
        log("Parser",
            QString("%1 troll exit mapping").arg(trollExitMapping ? "Enabling" : "Disabling"));
    }
}

namespace mume_xml_parser_gmcp_detail {

NODISCARD static CommandEnum getMove(const JsonObj &obj)
{
    if (auto dir = obj.getString("dir")) {
        if (verbose_debugging) {
            qInfo().noquote() << "MOVED" << *dir;
        }
        if (dir == "north") {
            return CommandEnum::NORTH;
        } else if (dir == "south") {
            return CommandEnum::SOUTH;
        } else if (dir == "east") {
            return CommandEnum::EAST;
        } else if (dir == "west") {
            return CommandEnum::WEST;
        } else if (dir == "up") {
            return CommandEnum::UP;
        } else if (dir == "down") {
            return CommandEnum::DOWN;
        } else if (dir == "none") {
            return CommandEnum::NONE;
        } else {
            qWarning().noquote() << "unknown movement direction:" << *dir;
            return CommandEnum::UNKNOWN;
        }
    }

    if (verbose_debugging) {
        qInfo().noquote() << "MOVED (unknown)";
    }
    return CommandEnum::UNKNOWN;
}

NODISCARD static RoomTerrainEnum getTerrain(const JsonObj &obj)
{
    const auto pEnv = obj.getString("environment");
    if (!pEnv) {
        return RoomTerrainEnum::UNDEFINED;
    }

    auto &env = *pEnv;
    if (env == "building") {
        return RoomTerrainEnum::INDOORS;
    } else if (env == "shallows") {
        return RoomTerrainEnum::SHALLOW;
    }

    const std::string s = mmqt::toStdStringUtf8(env);

    do {
#define X_CASE(x) \
    if (areEqualAsLowerUtf8(#x, s)) { \
        return RoomTerrainEnum::x; \
    }
        XFOREACH_RoomTerrainEnum(X_CASE)
#undef X_CASE
    } while (false);

    //
    qWarning() << "Unknown room terrain" << env;
    return RoomTerrainEnum::UNDEFINED;
}

NODISCARD static ServerRoomId asServerId(const JsonInt room)
{
    // CAUTION: static analysis is wrong here, because room is a signed integer,
    // so (room < 1) can happen.
    static_assert(std::is_signed_v<decltype(room)>);
    return (room < 1) ? INVALID_SERVER_ROOMID : ServerRoomId{static_cast<uint32_t>(room)};
}

NODISCARD static ServerRoomId getServerId(const JsonObj &obj)
{
    const auto optRoom = obj.getInt("id");
    if (!optRoom) {
        return INVALID_SERVER_ROOMID;
    }
    const JsonInt &room = *optRoom;
    if (verbose_debugging) {
        qInfo().noquote() << "ID:" << room;
    }
    return asServerId(room);
}

NODISCARD static RoomArea getRoomArea(const JsonObj &obj)
{
    if (auto area = obj.getString("area")) {
        if (verbose_debugging) {
            qInfo().noquote() << "Area:" << *area;
        }
        return mmqt::makeRoomArea(*area);
    }
    return RoomArea{};
}

NODISCARD static RoomName getRoomName(const JsonObj &obj)
{
    if (auto name = obj.getString("name")) { // can be null
        if (verbose_debugging) {
            qInfo().noquote() << "Name:" << *name;
        }
        return mmqt::makeRoomName(*name);
    }
    return RoomName{};
}

NODISCARD static RoomDesc getRoomDesc(const JsonObj &obj)
{
    if (auto desc = obj.getString("desc")) {
        if (verbose_debugging) {
            qInfo() << "Desc:" << *desc;
        }
        return mmqt::makeRoomDesc(*desc);
    }
    return RoomDesc{};
}

struct NODISCARD Misc final
{
    enum class DoorStateEnum { CLOSED, OPEN, BROKEN };
    using DoorStates = EnumIndexedArray<DoorStateEnum, ExitDirEnum, NUM_EXITS>;
    DoorStates doors{};
    RawExits exits{};
    ConnectedRoomFlagsType connectedRoomFlags{};
    ServerExitIds exitIds{};
};

static void processOneFlag(const QString &flag,
                           const ExitDirEnum d,
                           RawExit &exit,
                           ConnectedRoomFlagsType &connectedRoomFlags,
                           Misc::DoorStateEnum &door)
{
    if (flag == "hidden") {
        exit.addDoorFlags(DoorFlagEnum::HIDDEN);
    } else if (flag == "broken") {
        door = Misc::DoorStateEnum::BROKEN;
    } else if (flag == "closed") {
        door = Misc::DoorStateEnum::CLOSED;
    } else if (flag == "climb-down" || flag == "climb-up") {
        exit.addExitFlags(ExitFlagEnum::CLIMB);
    } else if (flag == "road" || flag == "trail") {
        exit.addExitFlags(ExitFlagEnum::ROAD);
    } else if (flag == "sundeath" || flag == "sunny") {
        connectedRoomFlags.setDirectSunlight(d, DirectSunlightEnum::SAW_DIRECT_SUN);
    } else if (flag == "water") {
        // TODO: Not useful as of now
    } else {
        const char dir[2] = {lowercaseDirection(d)[0], char_consts::C_NUL};
        qWarning().noquote() << "exit" << dir << "has unknown flag:" << flag;
    }
}

NODISCARD static Misc getMisc(const JsonObj &obj, const ServerRoomId room, bool isTrollMode)
{
    auto exits = obj.getObject("exits");
    if (!exits) {
        return {};
    }

    Misc result;
    for (const ExitDirEnum d : ALL_EXITS_NESWUD) {
        const char dir[2] = {lowercaseDirection(d)[0], char_consts::C_NUL};
        const auto optExit = exits->getObject(dir);
        if (!optExit) {
            continue;
        }

        RawExit &currentExit = result.exits[d];
        currentExit.addExitFlags(ExitFlagEnum::EXIT);

        const JsonObj &exit = *optExit;
        const auto optTo = exit.getInt("id");
        if (room != INVALID_SERVER_ROOMID && optTo) {
            const JsonInt &to = *optTo;
            if (verbose_debugging) {
                qInfo().noquote() << "EXIT from" << room.asUint32() << dir << "to"
                                  << asServerId(to).asUint32();
            }
            result.exitIds[d] = asServerId(to);
            // REVISIT: Add a ServerRawExit instead of using exitIds?
            // currentExit.getOutgoingSet().insert(asServerId(to));
        }

        const auto optDoorName = exit.getString("name");
        if (optDoorName) {
            const QString &doorName = *optDoorName;
            if (verbose_debugging) {
                qInfo().noquote() << "exit" << dir << "name:" << doorName;
            }
            currentExit.addExitFlags(ExitFlagEnum::DOOR);
            currentExit.setDoorName(mmqt::makeDoorName(doorName));
            auto &currentDoor = result.doors.at(d);
            currentDoor = Misc::DoorStateEnum::OPEN;
        }

        const auto optFlags = exit.getArray("flags");
        if (!optFlags) {
            continue;
        }

        for (const JsonValue &pflag : *optFlags) {
            if (auto optString = pflag.getString()) {
                const QString flag = optString.value();
                processOneFlag(flag, d, currentExit, result.connectedRoomFlags, result.doors.at(d));
            }
        }
    }

    // Orcs and trolls can detect exits with direct sunlight
    auto &connectedFlags = result.connectedRoomFlags;
    if (connectedFlags.hasAnyDirectSunlight() || isTrollMode) {
        for (const ExitDirEnum alt_dir : ALL_EXITS_NESWUD) {
            const auto &eThisExit = result.exits[alt_dir];
            const auto eThisClosed = eThisExit.exitIsDoor()
                                     && result.doors.at(alt_dir) == Misc::DoorStateEnum::CLOSED;

            // Do not flag indirect sunlight if there was a closed door, no exit, or we saw direct sunlight
            if (!eThisExit.exitIsExit() || eThisClosed
                || connectedFlags.hasDirectSunlight(alt_dir)) {
                continue;
            }

            // Flag indirect sun
            connectedFlags.setDirectSunlight(alt_dir, DirectSunlightEnum::SAW_NO_DIRECT_SUN);
        }
        connectedFlags.setValid();
    }
    if (isTrollMode) {
        connectedFlags.setTrollMode();
    }

    return result;
}

} // namespace mume_xml_parser_gmcp_detail

void MumeXmlParser::parseGmcpCharVitals(const JsonObj &obj)
{
    auto &promptFlags = m_commonData.promptFlags;
    using namespace mume_xml_parser_gmcp_detail;

    promptFlags.setValid();

    if (!obj.getNull("fog")) {
        if (verbose_debugging && promptFlags.getFogType() != PromptFogEnum::NO_FOG) {
            qInfo().noquote() << "fog null";
        }
        promptFlags.setFogType(PromptFogEnum::NO_FOG);
    } else if (auto fog = obj.getString("fog")) {
        if (verbose_debugging) {
            qInfo().noquote() << "fog" << *fog;
        }
        if (fog == mmqt::QS_MINUS_SIGN) {
            promptFlags.setFogType(PromptFogEnum::LIGHT_FOG);
        } else if (fog == mmqt::QS_EQUALS) {
            promptFlags.setFogType(PromptFogEnum::HEAVY_FOG);
        } else {
            qWarning().noquote() << "prompt has unknown fog flag:" << *fog;
        }
    }

    if (auto light = obj.getString("light")) {
        if (verbose_debugging) {
            qInfo().noquote() << "light" << *light;
        }
        if (light == mmqt::QS_ASTERISK           // indoor/sun (direct and indirect)
            || light == mmqt::QS_CLOSE_PARENS) { // moon (direct and indirect)
            promptFlags.setLit();
        } else if (light == "o") { // darkness (magical, night, or permanent)
            promptFlags.setDark();
        } else if (light == mmqt::QS_EXCLAMATION) { // artifical light
            promptFlags.setArtificial();
        } else {
            qWarning().noquote() << "prompt has unknown light flag:" << *light;
        }
    }

    if (!obj.getNull("weather")) {
        if (verbose_debugging && promptFlags.getWeatherType() != PromptWeatherEnum::NICE) {
            qInfo().noquote() << "weather null";
        }
        promptFlags.setWeatherType(PromptWeatherEnum::NICE);
    } else if (auto weather = obj.getString("weather")) {
        if (verbose_debugging) {
            qInfo().noquote() << "weather" << *weather;
        }
        if (weather == mmqt::QS_TILDE) {
            promptFlags.setWeatherType(PromptWeatherEnum::CLOUDS);
        } else if (weather == mmqt::QS_SQUOTE) {
            promptFlags.setWeatherType(PromptWeatherEnum::RAIN);
        } else if (weather == mmqt::QS_DQUOTE) {
            promptFlags.setWeatherType(PromptWeatherEnum::HEAVY_RAIN);
        } else if (weather == mmqt::QS_ASTERISK) {
            promptFlags.setWeatherType(PromptWeatherEnum::SNOW);
        } else if (weather != mmqt::QS_SPACE) {
            qWarning().noquote() << "prompt has unknown weather flag:" << *weather;
        }
    }
}

void MumeXmlParser::parseGmcpEventMoved(const JsonObj &obj)
{
    using namespace mume_xml_parser_gmcp_detail;
    const CommandEnum move = getMove(obj);
    setMove(move);
}

void MumeXmlParser::parseGmcpRoomInfo(const JsonObj &obj)
{
    using namespace mume_xml_parser_gmcp_detail;
    m_serverId = getServerId(obj);

    m_commonData.terrain = getTerrain(obj);
    m_commonData.roomArea = getRoomArea(obj);
    m_commonData.roomName = getRoomName(obj);
    m_commonData.roomDesc = getRoomDesc(obj);

    const auto misc = getMisc(obj, m_serverId, m_commonData.trollExitMapping);
    m_commonData.connectedRoomFlags = misc.connectedRoomFlags;
    m_commonData.roomExits = misc.exits;
    m_commonData.exitIds = misc.exitIds;

    m_eventReady = true;
}
