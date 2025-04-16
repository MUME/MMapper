// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../configuration/configuration.h"
#include "../global/CaseUtils.h"
#include "../global/progresscounter.h"
#include "../group/mmapper2group.h"
#include "../map/ExitsFlags.h"
#include "../map/ParseTree.h"
#include "../map/PromptFlags.h"
#include "../map/parseevent.h"
#include "../mapdata/mapdata.h"
#include "../proxy/GmcpMessage.h"
#include "abstractparser.h"
#include "mumexmlparser.h"
#include "patterns.h"

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
        trollExitMapping = (race->compare("Troll", Qt::CaseInsensitive) == 0);
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
    ExitsFlagsType exitsFlags{};
    // PromptFlagsType promptFlags{};
    ConnectedRoomFlagsType connectedRoomFlags{};
    ServerExitIds exitIds{};
};

static void processOneFlag(const QString &flag, const ExitDirEnum d, Misc &result)
{
    if (flag == "broken" || flag == "closed" || flag == "hidden") {
        result.exitsFlags.set(d, ExitFlagEnum::DOOR);
    } else if (flag == "climb-down" || flag == "climb-up") {
        result.exitsFlags.set(d, ExitFlagEnum::CLIMB);
    } else if (flag == "road" || flag == "trail") {
        result.exitsFlags.set(d, ExitFlagEnum::ROAD);
    } else if (flag == "sundeath" || flag == "sunny") {
        result.connectedRoomFlags.setValid();
        result.connectedRoomFlags.setDirectSunlight(d, DirectSunlightEnum::SAW_DIRECT_SUN);
    } else if (flag == "water") {
    } else {
        const char dir[2] = {lowercaseDirection(d)[0], char_consts::C_NUL};
        qWarning().noquote() << "exit" << dir << "has unknown flag:" << flag;
    }
}

NODISCARD static Misc getMisc(const JsonObj &obj, const ServerRoomId room)
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

        result.exitsFlags.setValid();
        result.exitsFlags.set(d, ExitFlagEnum::EXIT);

        const JsonObj &exit = *optExit;
        const auto optTo = exit.getInt("id");
        if (room != INVALID_SERVER_ROOMID && optTo) {
            const JsonInt &to = *optTo;
            if (verbose_debugging) {
                qInfo().noquote() << "EXIT from" << room.asUint32() << dir << "to"
                                  << asServerId(to).asUint32();
            }
            result.exitIds[d] = asServerId(to);
        }

        const auto optDoorName = exit.getString("name");
        if (optDoorName) {
            const QString &doorName = *optDoorName;
            if (verbose_debugging) {
                qInfo().noquote() << "exit" << dir << "name:" << doorName;
            }
        }

        const auto optFlags = exit.getArray("flags");
        if (!optFlags) {
            continue;
        }

        for (const JsonValue &pflag : *optFlags) {
            if (auto optString = pflag.getString()) {
                const QString flag = optString.value();
                processOneFlag(flag, d, result);
            }
        }
    }

    return result;
}

} // namespace mume_xml_parser_gmcp_detail

void MumeXmlParser::parseGmcpCharVitals(const JsonObj &obj)
{
    auto &promptFlags = m_commonData.promptFlags;
    using namespace mume_xml_parser_gmcp_detail;
    if (auto fog = obj.getString("fog")) {
        if (verbose_debugging) {
            qInfo().noquote() << "fog" << *fog;
        }
        if (fog == "-") {
            promptFlags.setFogType(PromptFogEnum::LIGHT_FOG);
        } else if (fog == "=") {
            promptFlags.setFogType(PromptFogEnum::HEAVY_FOG);
        } else {
            qWarning().noquote() << "prompt has unknown fog flag:" << *fog;
        }
        promptFlags.setValid();
    }

    if (auto light = obj.getString("light")) {
        if (verbose_debugging) {
            qInfo().noquote() << "light" << *light;
        }
        if (light == mmqt::QS_ASTERISK // indoor/sun (direct and indirect)
            || light == ")") {         // moon (direct and indirect)
            promptFlags.setLit();
        } else if (light == "o") { // darkness
            promptFlags.setDark();
        } else if (light != "!") { // ignore artifical light
            qWarning().noquote() << "prompt has unknown light flag:" << *light;
        }
        promptFlags.setValid();
    }

    if (auto weather = obj.getString("weather")) {
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
        promptFlags.setValid();
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
    m_roomName = getRoomName(obj);
    m_roomDesc = getRoomDesc(obj);
    const auto misc = getMisc(obj, m_serverId);
    m_commonData.connectedRoomFlags = misc.connectedRoomFlags;
    m_commonData.exitsFlags = misc.exitsFlags;
    m_exitIds = misc.exitIds;
    //m_promptFlags = misc.promptFlags;
}
