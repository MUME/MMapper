// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "abstractparser.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <QDesktopServices>
#include <QMessageLogContext>
#include <QObject>
#include <QVariant>
#include <QtCore>

#include "../clock/mumeclock.h"
#include "../clock/mumemoment.h"
#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/CharBuffer.h"
#include "../global/RAII.h"
#include "../global/StringView.h"
#include "../global/TextUtils.h"
#include "../global/random.h"
#include "../global/utils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/RoomFieldVariant.h"
#include "../mapdata/enums.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomfilter.h"
#include "../mapdata/roomselection.h"
#include "../mapdata/shortestpath.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/GmcpUtils.h"
#include "../proxy/proxy.h"
#include "../proxy/telnetfilter.h"
#include "../syntax/Accept.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TokenMatcher.h"
#include "../syntax/TreeParser.h"
#include "Abbrev.h"
#include "AbstractParser-Commands.h"
#include "AbstractParser-Utils.h"
#include "CommandId.h"
#include "CommandQueue.h"
#include "ConnectedRoomFlags.h"
#include "DoorAction.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"
#include "parserutils.h"

class RoomAdmin;

const QString AbstractParser::nullString{};
const QByteArray AbstractParser::emptyByteArray("");

static char getTerrainSymbol(const RoomTerrainEnum type)
{
    switch (type) {
    case RoomTerrainEnum::UNDEFINED:
        return ' ';
    case RoomTerrainEnum::INDOORS:
        return '['; // [  // indoors
    case RoomTerrainEnum::CITY:
        return '#'; // #  // city
    case RoomTerrainEnum::FIELD:
        return '.'; // .  // field
    case RoomTerrainEnum::FOREST:
        return 'f'; // f  // forest
    case RoomTerrainEnum::HILLS:
        return '('; // (  // hills
    case RoomTerrainEnum::MOUNTAINS:
        return '<'; // <  // mountains
    case RoomTerrainEnum::SHALLOW:
        return '%'; // %  // shallow
    case RoomTerrainEnum::WATER:
        return '~'; // ~  // water
    case RoomTerrainEnum::RAPIDS:
        return 'W'; // W  // rapids
    case RoomTerrainEnum::UNDERWATER:
        return 'U'; // U  // underwater
    case RoomTerrainEnum::ROAD:
        return '+'; // +  // road
    case RoomTerrainEnum::TUNNEL:
        return '='; // =  // tunnel
    case RoomTerrainEnum::CAVERN:
        return 'O'; // O  // cavern
    case RoomTerrainEnum::BRUSH:
        return ':'; // :  // brush
    case RoomTerrainEnum::DEATHTRAP:
        return 'X';
    };

    return ' ';
}

static char getLightSymbol(const RoomLightEnum lightType)
{
    switch (lightType) {
    case RoomLightEnum::DARK:
        return 'o';
    case RoomLightEnum::LIT:
    case RoomLightEnum::UNDEFINED:
        return '*';
    }

    return '?';
}

AbstractParser::AbstractParser(MapData *const md,
                               MumeClock *const mc,
                               ProxyParserApi proxy,
                               GroupManagerApi group,
                               QObject *const parent)
    : QObject(parent)
    , m_mumeClock(mc)
    , m_mapData(md)
    , m_proxy(std::move(proxy))
    , m_group(std::move(group))
    , prefixChar{getConfig().parser.prefixChar}
{
    connect(&m_offlineCommandTimer, &QTimer::timeout, this, &AbstractParser::doOfflineCharacterMove);

    // MUME only attempts up to 4 commands per second (i.e. 250ms)
    m_offlineCommandTimer.setInterval(250);
    m_offlineCommandTimer.setSingleShot(true);

    initSpecialCommandMap();
    initActionMap();
}

AbstractParser::~AbstractParser() = default;

void AbstractParser::reset()
{
    if (m_trollExitMapping) {
        emit log("Parser", "Disabling troll exit mapping");
        m_trollExitMapping = false;
    }
    m_lastPrompt = "";
    m_queue.clear();
}

void AbstractParser::parsePrompt(const QString &prompt)
{
    m_promptFlags.reset();

    if (prompt.length() < 2)
        return;

    int index = 0;
    if (prompt[index] == '+') {
        // susceptible to mudlle (Valar only)
        index += 2;
    }
    switch (prompt[index++].toLatin1()) {
    case '*': // indoor/sun (direct and indirect)
        m_promptFlags.setLit();
        break;
    case '!': // artifical light
        break;
    case ')': // moon (direct and indirect)
        m_promptFlags.setLit();
        break;
    case 'o': // darkness
        m_promptFlags.setDark();
        break;
    default:
        index--;
    }

    switch (prompt[index++].toLatin1()) {
    case '[':
        m_promptFlags.setTerrainType(RoomTerrainEnum::INDOORS);
        break;
    case '#':
        m_promptFlags.setTerrainType(RoomTerrainEnum::CITY);
        break;
    case '.':
        m_promptFlags.setTerrainType(RoomTerrainEnum::FIELD);
        break;
    case 'f':
        m_promptFlags.setTerrainType(RoomTerrainEnum::FOREST);
        break;
    case '(':
        m_promptFlags.setTerrainType(RoomTerrainEnum::HILLS);
        break;
    case '<':
        m_promptFlags.setTerrainType(RoomTerrainEnum::MOUNTAINS);
        break;
    case '%':
        m_promptFlags.setTerrainType(RoomTerrainEnum::SHALLOW);
        break;
    case '~':
        m_promptFlags.setTerrainType(RoomTerrainEnum::WATER);
        break;
    case 'W':
        m_promptFlags.setTerrainType(RoomTerrainEnum::RAPIDS);
        break;
    case 'U':
        m_promptFlags.setTerrainType(RoomTerrainEnum::UNDERWATER);
        break;
    case '+':
        m_promptFlags.setTerrainType(RoomTerrainEnum::ROAD);
        break;
    case '=':
        m_promptFlags.setTerrainType(RoomTerrainEnum::TUNNEL);
        break;
    case 'O':
        m_promptFlags.setTerrainType(RoomTerrainEnum::CAVERN);
        break;
    case ':':
        m_promptFlags.setTerrainType(RoomTerrainEnum::BRUSH);
        break;
    default:
        index--;
        break;
    }

    if (index < prompt.length()) {
        switch (prompt[index++].toLatin1()) {
        case '~':
            m_promptFlags.setWeatherType(PromptWeatherEnum::CLOUDS);
            break;
        case '\'':
            m_promptFlags.setWeatherType(PromptWeatherEnum::RAIN);
            break;
        case '"':
            m_promptFlags.setWeatherType(PromptWeatherEnum::HEAVY_RAIN);
            break;
        case '*':
            m_promptFlags.setWeatherType(PromptWeatherEnum::SNOW);
            break;
        default:
            index--;
            break;
        }
    }

    if (index < prompt.length()) {
        switch (prompt[index++].toLatin1()) {
        case '-':
            m_promptFlags.setFogType(PromptFogEnum::LIGHT_FOG);
            break;
        case '=':
            m_promptFlags.setFogType(PromptFogEnum::HEAVY_FOG);
            break;
        default:
            index--;
            break;
        }
    }

    m_promptFlags.setValid();
}

void AbstractParser::parseExits(std::ostream &os)
{
    QString str = normalizeStringCopy(m_exits);
    m_connectedRoomFlags.reset();
    m_exitsFlags.reset();
    ExitsFlagsType closedDoorFlag;
    bool doors = false;
    bool closed = false;
    bool road = false;
    bool climb = false;
    bool portal = false;
    bool directSun = false;
    auto dir = ExitDirEnum::UNKNOWN;

    const auto reset_exit_flags = [&doors, &closed, &road, &climb, &portal, &directSun, &dir]() {
        doors = false;
        closed = false;
        road = false;
        climb = false;
        portal = false;
        directSun = false;
        dir = ExitDirEnum::UNKNOWN;
    };

    const auto set_exit_flags =
        [this, &closedDoorFlag, &climb, &doors, &closed, &road, &directSun, &dir]() {
            if (isNESWUD(dir)) {
                ExitFlags exitFlags{ExitFlagEnum::EXIT};
                if (climb) {
                    exitFlags |= ExitFlagEnum::CLIMB;
                }
                if (doors) {
                    exitFlags |= ExitFlagEnum::DOOR;
                    if (closed) {
                        closedDoorFlag.set(dir, ExitFlagEnum::DOOR);
                    }
                }
                if (road) {
                    exitFlags |= ExitFlagEnum::ROAD;
                }
                if (directSun) {
                    setConnectedRoomFlag(DirectSunlightEnum::SAW_DIRECT_SUN, dir);
                }
                setExitFlags(exitFlags, dir);
            }
        };

    const auto parse_exit_flag =
        [this, &doors, &closed, &road, &climb, &portal, &directSun](const char sign) -> bool {
        switch (sign) {
        case '(': // open door
            doors = true;
            break;
        case '[': // closed door
            doors = true;
            closed = true;
            break;
        case '#': // broken door
            doors = true;
            break;
        case '=': // road
            road = true;
            break;
        case '-': // trail
            road = true;
            break;
        case '/': // upward climb
            climb = true;
            break;
        case '\\': // downward climb
            climb = true;
            break;
        case '{': // portal
            portal = true;
            break;
        case '*': // sunlit room (troll/orc only)
            directSun = true;
            break;
        case '^': // outdoors room (troll only)
            directSun = true;
            if (!m_trollExitMapping) {
                emit log("Parser", "Autoenabling troll exit mapping mode.");
            }
            m_trollExitMapping = true;
            break;
        default:
            return false;
        }
        return true;
    };

    if (str.length() > 5 && str.at(5).toLatin1() != ':') {
        // Ainur exits
        static QRegularExpression rx(
            R"(^\s*([\^\*~\-={#\[\(\\\/]+)?([A-za-z]+)([\^\*~\-=}#\]\)\\\/]+)?\s+\- (.*)$)");
        static QRegularExpression newline(R"(\r?\n)");
        for (const auto &part : str.split(newline)) {
            const auto match = rx.match(part);
            if (match.hasMatch()) {
                // Parse exit flag
                const auto &signs = match.captured(1);
                for (const QChar &qc : signs) {
                    parse_exit_flag(qc.toLatin1());
                }

                // Set exit flags to direction
                const auto &exit = match.captured(2);
                dir = Mmapper2Exit::dirForChar(exit.at(0).toLower().toLatin1());
                set_exit_flags();

                // Reset for next exit
                reset_exit_flags();
            }
        }
    } else {
        // Player exits
        const int length = str.length();
        for (int i = 7; i < length; i++) {
            const char c = str.at(i).toLatin1();
            if (!parse_exit_flag(c)) {
                switch (c) {
                case ' ': // empty space means reset for next exit
                    reset_exit_flags();
                    break;

                case 'n':
                    if ((i + 2) < length && (str.at(i + 2).toLatin1()) == 'r') { // north
                        i += 5;
                        dir = ExitDirEnum::NORTH;
                    } else {
                        i += 4; // none
                        dir = ExitDirEnum::NONE;
                    }
                    break;

                case 's':
                    i += 5;
                    dir = ExitDirEnum::SOUTH;
                    break;

                case 'e':
                    i += 4;
                    dir = ExitDirEnum::EAST;
                    break;

                case 'w':
                    i += 4;
                    dir = ExitDirEnum::WEST;
                    break;

                case 'u':
                    i += 2;
                    dir = ExitDirEnum::UP;
                    break;

                case 'd':
                    i += 4;
                    dir = ExitDirEnum::DOWN;
                    break;
                default:;
                }
                set_exit_flags();
            }
        }
    }

    // If there is't a portal then we can trust the exits
    if (!portal) {
        m_exitsFlags.setValid();

        // Trolls can detect exits with direct sunlight
        if (m_trollExitMapping) {
            m_connectedRoomFlags.setValid();
            for (const auto alt_dir : ALL_EXITS_NESWUD) {
                const auto eThisExit = m_exitsFlags.get(alt_dir);
                const auto eThisClosed = closedDoorFlag.get(alt_dir);

                // Do not flag indirect sunlight if there was a closed door, no exit, or we saw direct sunlight
                if (!eThisExit.isExit() || eThisClosed.isDoor()
                    || m_connectedRoomFlags.hasDirectSunlight(alt_dir)) {
                    continue;
                }

                // Flag indirect sun
                setConnectedRoomFlag(DirectSunlightEnum::SAW_NO_DIRECT_SUN, alt_dir);
            }
        }
    }

    RoomSelection rs(*m_mapData);
    if (const Room *const room = rs.getRoom(getNextPosition())) {
        const QByteArray cn = enhanceExits(room);
        const auto right_trim = [](const QString &str) -> QString {
            for (int n = str.size() - 1; n >= 0; --n) {
                if (!str.at(n).isSpace()) {
                    return str.left(n + 1);
                }
            }
            return "";
        };
        os << ::toStdStringLatin1(right_trim(m_exits) + cn);

        if (getConfig().mumeNative.showNotes) {
            const auto &ns = room->getNote();
            if (!ns.isEmpty()) {
                const QString note = QString("Note: %1\r\n").arg(ns.toQString());
                os << ::toStdStringLatin1(note);
            }
        }
    } else {
        os << ::toStdStringLatin1(m_exits);
    }
}

QString AbstractParser::normalizeStringCopy(QString string)
{
    // Remove ANSI first, since we don't want Latin1
    // transliterations to accidentally count as ANSI.
    ParserUtils::removeAnsiMarksInPlace(string);
    ParserUtils::toAsciiInPlace(string);
    return string;
}

const Coordinate AbstractParser::getNextPosition()
{
    CommandQueue tmpqueue;
    if (!m_queue.isEmpty()) {
        // Next position in the prespammed path
        tmpqueue.enqueue(m_queue.head());
    }

    QList<Coordinate> cl = m_mapData->getPath(m_mapData->getPosition(), tmpqueue);
    return cl.isEmpty() ? m_mapData->getPosition() : cl.back();
}

const Coordinate AbstractParser::getTailPosition()
{
    // Position at the end of the prespammed path
    QList<Coordinate> cl = m_mapData->getPath(m_mapData->getPosition(), m_queue);
    return cl.isEmpty() ? m_mapData->getPosition() : cl.back();
}

void AbstractParser::emulateExits(std::ostream &os, const CommandEnum move)
{
    const auto nextCoordinate = [this, &move]() {
        // Use movement direction to find the next coordinate
        if (isDirectionNESWUD(move)) {
            auto rs = RoomSelection(*m_mapData);
            if (const Room *const sourceRoom = rs.getRoom(m_mapData->getPosition())) {
                const auto &exit = sourceRoom->exit(getDirection(move));
                if (exit.isExit() && !exit.outIsEmpty())
                    if (const Room *const targetRoom = rs.getRoom(exit.outFirst()))
                        return targetRoom->getPosition();
            }
        }
        // Fallback to next position in prespammed path
        return getNextPosition();
    }();
    auto rs = RoomSelection(*m_mapData);
    if (const Room *const r = rs.getRoom(nextCoordinate))
        sendRoomExitsInfoToUser(os, r);
}

QByteArray AbstractParser::enhanceExits(const Room *sourceRoom)
{
    QByteArray cn = " -";
    bool enhancedExits = false;

    auto sourceId = sourceRoom->getId();
    for (auto i : ALL_EXITS_NESWUD) {
        const Exit &e = sourceRoom->exit(i);
        const ExitFlags ef = e.getExitFlags();
        if (!ef.isExit()) {
            continue;
        }

        QByteArray etmp;
        auto add_exit_keyword = [&etmp](const char *word) {
            if (!etmp.isEmpty()) {
                etmp += ",";
            }
            etmp += word;
        };

        // Extract hidden exit flags
        if (getConfig().mumeNative.showHiddenExitFlags) {
            /* TODO: const char* lowercaseName(ExitFlagEnum) */
            if (ef.contains(ExitFlagEnum::NO_FLEE)) {
                add_exit_keyword("noflee");
            }
            if (ef.contains(ExitFlagEnum::RANDOM)) {
                add_exit_keyword("random");
            }
            if (ef.contains(ExitFlagEnum::SPECIAL)) {
                add_exit_keyword("special");
            }
            if (ef.contains(ExitFlagEnum::DAMAGE)) {
                add_exit_keyword("damage");
            }
            if (ef.contains(ExitFlagEnum::FALL)) {
                add_exit_keyword("fall");
            }
            if (ef.contains(ExitFlagEnum::GUARDED)) {
                add_exit_keyword("guarded");
            }

            // Exit modifiers
            if (e.containsOut(sourceId)) {
                add_exit_keyword("loop");

            } else if (!e.outIsEmpty()) {
                // Check target room for exit information
                const auto targetId = e.outFirst();
                uint exitCount = 0;
                bool oneWay = false;
                bool hasNoFlee = false;
                auto rs = RoomSelection(*m_mapData);
                if (const Room *const targetRoom = rs.getRoom(targetId)) {
                    if (!targetRoom->exit(opposite(i)).containsOut(sourceId)) {
                        oneWay = true;
                    }
                    for (auto j : ALL_EXITS_NESWUD) {
                        const Exit &targetExit = targetRoom->exit(j);
                        if (!targetExit.getExitFlags().isExit()) {
                            continue;
                        }
                        exitCount++;
                        if (targetExit.containsOut(sourceId)) {
                            // Technically rooms can point back in a different direction
                            oneWay = false;
                        }
                        if (targetExit.getExitFlags().contains(ExitFlagEnum::NO_FLEE)) {
                            hasNoFlee = true;
                        }
                    }
                    if (oneWay) {
                        add_exit_keyword("oneway");
                    }
                    if (hasNoFlee && exitCount == 1) {
                        // If there is only 1 exit out of this room add the 'hasnoflee' flag since its usually a mobtrap
                        add_exit_keyword("hasnoflee");
                    }

                    // Terrain type exit modifiers
                    RoomTerrainEnum targetTerrain = targetRoom->getTerrainType();
                    if (targetTerrain == RoomTerrainEnum::UNDERWATER) {
                        add_exit_keyword("underwater");
                    } else if (targetTerrain == RoomTerrainEnum::DEATHTRAP) {
                        // Override all previous flags
                        etmp = "deathtrap";
                    }
                }
            }
        }

        // Extract door names
        QByteArray dn = e.getDoorName().toQByteArray();
        if (!dn.isEmpty() || !etmp.isEmpty()) {
            enhancedExits = true;
            cn += " ";
            cn += Mmapper2Exit::charForDir(i);
            cn += ":";
            if (!dn.isEmpty()) {
                cn += dn;
            }
            if (!etmp.isEmpty()) {
                cn += "(" + etmp + ")";
            }
        }
    }

    if (!enhancedExits) {
        return "\r\n";
    }

    cn += ".\r\n";
    return cn;
}

void AbstractParser::parseNewUserInput(const TelnetData &data)
{
    auto parse_and_send = [this, &data]() {
        auto parse = [this, &data]() -> bool {
            const QString input = QString::fromLatin1(data.line.constData(), data.line.size())
                                      .simplified();
            try {
                return parseUserCommands(input);
            } catch (const std::exception &ex) {
                qWarning() << "Exception: " << ex.what();
                sendToUser(QString::asprintf("An exception occurred: %s\r\n", ex.what()));
                sendPromptToUser();
                return false;
            }
        };

        if (parse()) {
            emit sendToMud(data.line);
        }
    };

    switch (data.type) {
    case TelnetDataEnum::DELAY:
    case TelnetDataEnum::PROMPT:
    case TelnetDataEnum::MENU_PROMPT:
    case TelnetDataEnum::LOGIN:
    case TelnetDataEnum::LOGIN_PASSWORD:
    case TelnetDataEnum::UNKNOWN:
        emit sendToMud(data.line);
        break;
    case TelnetDataEnum::CRLF:
    case TelnetDataEnum::LF:
        parse_and_send();
        break;
    }
}

static QString compressDirections(QString original)
{
    QString ans;
    int curnum = 0;
    QChar curval = '\0';
    Coordinate delta;
    const auto addDirs = [&ans, &curnum, &curval, &delta]() {
        assert(curnum >= 1);
        assert(curval != '\0');
        if (curnum > 1) {
            ans.append(QString::number(curnum));
        }
        ans.append(curval);

        const auto dir = Mmapper2Exit::dirForChar(curval.toLatin1());
        delta += Room::exitDir(dir) * curnum;
    };

    for (auto c : original) {
        if (curnum != 0 && curval == c) {
            curnum++;
        } else {
            if (curnum != 0)
                addDirs();
            curnum = 1;
            curval = c;
        }
    }
    if (curnum != 0)
        addDirs();

    bool wantDelta = true;
    if (wantDelta) {
        auto addNumber =
            [&curnum, &curval, &addDirs, &ans](const int n, const char pos, const char neg) {
                if (n == 0)
                    return;
                curnum = std::abs(n);
                curval = (n < 0) ? neg : pos;
                ans += ' ';
                addDirs();
            };

        if (delta.isNull()) {
            ans += " (here)";
        } else {
            ans += " (total:";
            addNumber(delta.x, 'e', 'w');
            addNumber(delta.y, 'n', 's');
            addNumber(delta.z, 'u', 'd');
            ans += ")";
        }
    }

    return ans;
}

class ShortestPathEmitter final : public ShortestPathRecipient
{
    AbstractParser &parser;

public:
    explicit ShortestPathEmitter(AbstractParser &parser)
        : parser(parser)
    {}
    virtual ~ShortestPathEmitter() override;

    void receiveShortestPath(RoomAdmin * /*admin*/, QVector<SPNode> spnodes, int endpoint) override
    {
        const SPNode *spnode = &spnodes[endpoint];
        auto name = spnode->r->getName();
        parser.sendToUser("Distance " + QString::number(spnode->dist) + ": " + name + "\r\n");
        QString dirs;
        while (spnode->parent >= 0) {
            if (&spnodes[spnode->parent] == spnode) {
                parser.sendToUser("ERROR: loop\r\n");
                break;
            }
            dirs.append(Mmapper2Exit::charForDir(spnode->lastdir));
            spnode = &spnodes[spnode->parent];
        }
        std::reverse(dirs.begin(), dirs.end());
        parser.sendToUser("dirs: " + compressDirections(dirs) + "\r\n");
    }
};

ShortestPathEmitter::~ShortestPathEmitter() = default;

void AbstractParser::searchCommand(const RoomFilter &f)
{
    if (f.patternKind() == PatternKindsEnum::NONE) {
        emit newRoomSelection(SigRoomSelection{});
        sendToUser("Rooms unselected.\r\n");
        return;
    }
    const auto tmpSel = RoomSelection::createSelection(*m_mapData);
    tmpSel->genericSearch(f);
    sendToUser(
        QString("%1 room%2 found.\r\n").arg(tmpSel->size()).arg((tmpSel->size() == 1) ? "" : "s"));
    emit newRoomSelection(SigRoomSelection{tmpSel});
}

void AbstractParser::dirsCommand(const RoomFilter &f)
{
    ShortestPathEmitter sp_emitter(*this);

    auto rs = RoomSelection(*m_mapData);
    if (const Room *const r = rs.getRoom(getTailPosition())) {
        m_mapData->shortestPathSearch(r, &sp_emitter, f, 10, 0);
    }
}

void AbstractParser::markCurrentCommand()
{
    const auto tmpSel = RoomSelection::createSelection(*m_mapData, getTailPosition());
    emit newRoomSelection(SigRoomSelection{tmpSel});
}

ExitDirEnum AbstractParser::tryGetDir(StringView &view)
{
    if (view.isEmpty())
        return ExitDirEnum::UNKNOWN;

    auto word = view.takeFirstWord();
    for (auto dir : ALL_EXITS_NESWUD) {
        auto lower = lowercaseDirection(dir);
        if (lower != nullptr && Abbrev{lower, 1}.matches(word))
            return dir;
    }

    sendToUser(QString("Unexpected direction: \"%1\"\r\n").arg(word.toQString()));
    throw std::runtime_error("bad direction");
}

void AbstractParser::showCommandPrefix()
{
    const auto quote = static_cast<char>((prefixChar == '\'') ? '"' : '\'');
    sendToUser(QString("The current command prefix is: %1%2%1 (e.g. %2help) \r\n")
                   .arg(quote)
                   .arg(prefixChar));
}

bool AbstractParser::setCommandPrefix(const char prefix)
{
    if (!isValidPrefix(prefix))
        return false;
    setConfig().parser.prefixChar = prefix;
    showCommandPrefix();
    return true;
}

void AbstractParser::showSyntax(const char *rest)
{
    sendToUser(QString::asprintf("Usage: %c%s\r\n", prefixChar, rest));
}

void AbstractParser::showNote()
{
    printRoomInfo(RoomFieldEnum::NOTE);
}

void AbstractParser::toggleTrollMapping()
{
    m_trollExitMapping = !m_trollExitMapping;
    QString toggleText = enabledString(m_trollExitMapping);
    sendToUser("Troll exit mapping is now " + toggleText + ".\r\n");
}

void AbstractParser::doSearchCommand(StringView view)
{
    if (std::optional<RoomFilter> optFilter = RoomFilter::parseRoomFilter(view.getStdStringView())) {
        searchCommand(optFilter.value());
    } else {
        sendToUser(RoomFilter::parse_help);
    }
}

void AbstractParser::doGetDirectionsCommand(StringView view)
{
    if (std::optional<RoomFilter> optFilter = RoomFilter::parseRoomFilter(view.getStdStringView())) {
        dirsCommand(optFilter.value());
    } else {
        sendToUser(RoomFilter::parse_help);
    }
}

void AbstractParser::doMarkCurrentCommand()
{
    markCurrentCommand();
    sendToUser("--->Current room marked temporarily on the map.\r\n");
}

void AbstractParser::doRemoveDoorNamesCommand()
{
    m_mapData->removeDoorNames();
    sendToUser("Secret exits purged.\r\n");
}

void AbstractParser::doBackCommand()
{
    m_queue.clear();
    sendOkToUser();
    pathChanged();
}

void AbstractParser::doConnectToHost()
{
    m_proxy.connectToMud();
}

void AbstractParser::doDisconnectFromHost()
{
    m_proxy.disconnectFromMud();
}

void AbstractParser::openVoteURL()
{
    QDesktopServices::openUrl(QUrl(
        "https://www.mudconnect.com/cgi-bin/search.cgi?mode=mud_listing&mud=MUME+-+Multi+Users+In+Middle+Earth"));
    QDesktopServices::openUrl(QUrl("http://www.topmudsites.com/vote-MUME.html"));
    sendToUser("--->Thank you kindly for voting!\r\n");
}

void AbstractParser::showHelpCommands(const bool showAbbreviations)
{
    auto &map = this->m_specialCommandMap;

    struct record
    {
        std::string from;
        std::string to;
    };

    std::vector<record> records;
    for (auto &kv : map) {
        const auto &from = kv.first;
        const auto &to = kv.second.fullCommand;
        if (from.empty() || to.empty())
            throw std::runtime_error("internal havoc");

        if (showAbbreviations || from == to)
            records.emplace_back(record{from, to});
    }

    if (records.empty())
        return;

    std::sort(std::begin(records), std::end(records), [](const record &a, const record &b) {
        return a.from < b.from;
    });

    char currentLetter = records[0].from[0];
    for (const auto &rec : records) {
        const auto thisLetter = rec.from[0];
        if (thisLetter != currentLetter) {
            currentLetter = thisLetter;
            sendToUser("\r\n");
        }

        if (rec.from == rec.to)
            sendToUser(QString::asprintf("  %c%s\r\n", prefixChar, rec.from.c_str()));
        else
            sendToUser(QString::asprintf("  %c%-20s -> %c%s\r\n",
                                         prefixChar,
                                         rec.from.c_str(),
                                         prefixChar,
                                         rec.to.c_str()));
    }
}

void AbstractParser::showGroupHelp()
{
    showHeader("MMapper group manager help");
    showHeader("Group commands");
    sendToUser(QString("  %1GKick [player]      - kick [player] from the group\r\n"
                       "  %1GLock               - Toggle lock on group\r\n"
                       "  %1GTell [message]     - send a grouptell with the [message]\r\n")
                   .arg(prefixChar));
}

void AbstractParser::showHeader(const QString &s)
{
    QString result;
    result += "\r\n";
    result += s;
    result += "\r\n";
    result += QString(s.size(), '-');
    result += "\r\n";
    sendToUser(result);
}

void AbstractParser::showMiscHelp()
{
    showHeader("Miscellaneous commands");
    sendToUser(QString("  %1back        - delete prespammed commands from queue\r\n"
                       "  %1markcurrent - select the room you are currently in\r\n"
                       "  %1time        - display current MUME time\r\n"
                       "  %1trollexit   - toggle troll-only exit mapping for direct sunlight\r\n"
                       "  %1vote        - vote for MUME on TMC!\r\n")
                   .arg(prefixChar));
}

void AbstractParser::showHelp()
{
    auto s = QString("\r\nMMapper help:\r\n-------------\r\n"
                     "\r\nRoom mapping commands:\r\n"
                     "  %1room        - (see \"%1room ??\" for syntax help)\r\n"
                     "\r\nGroup commands:\r\n"
                     "  %1group       - (see \"%1group ??\" for syntax help)\r\n"
                     "  %1gtell       - send group tell\r\n"
                     "\r\nConfig commands:\r\n"
                     "  %1config      - (see \"%1config ??\" for syntax help)\r\n"
                     "\r\nHelp commands:\n"
                     "  %1help      - this help text\r\n"
                     "  %1help ??   - full syntax help for the help command\r\n"
                     "  %1doorhelp  - help for door console commands\r\n"
                     "\r\nOther commands:\n"
                     "  %1dirs [-options] pattern   - directions to matching rooms\r\n"
                     "  %1search [-options] pattern - select matching rooms\r\n"
                     "  %1set [prefix [punct-char]] - change command prefix\r\n"
                     "  %1connect                   - connect to the MUD\r\n"
                     "  %1disconnect                - disconnect from the MUD\r\n");

    sendToUser(s.arg(prefixChar));
}

void AbstractParser::showMumeTime()
{
    const MumeMoment moment = m_mumeClock->getMumeMoment();
    QByteArray data = m_mumeClock->toMumeTime(moment).toLatin1() + "\r\n";
    const MumeClockPrecisionEnum precision = m_mumeClock->getPrecision();
    if (precision > MumeClockPrecisionEnum::DAY) {
        const MumeTimeEnum time = moment.toTimeOfDay();
        data += "It is currently ";
        if (time == MumeTimeEnum::DAWN) {
            data += "\033[31mdawn\033[0m";
        } else if (time == MumeTimeEnum::DUSK) {
            data += "\033[34mdusk\033[0m and will be night";
        } else if (time == MumeTimeEnum::NIGHT) {
            data += "\033[34mnight\033[0m";
        } else {
            data += "\033[33mday\033[0m";
        }
        data += " for " + m_mumeClock->toCountdown(moment).toLatin1() + " more ticks.\r\n";

        // Moon data
        data += moment.toMumeMoonTime().toLatin1() + "\r\n";
        data += "The moon ";
        switch (moment.toMoonVisibility()) {
        case MumeMoonVisibilityEnum::HIDDEN:
        case MumeMoonVisibilityEnum::POSITION_UNKNOWN:
            data += "will rise in";
            break;
        case MumeMoonVisibilityEnum::RISE:
        case MumeMoonVisibilityEnum::SET:
        case MumeMoonVisibilityEnum::VISIBLE:
            data += "will set in";
            break;
        };
        data += " " + moment.toMoonCountDown().toLatin1() + " more ticks.\r\n";
    }
    sendToUser(data);
}

void AbstractParser::showDoorCommandHelp()
{
    showHeader("MMapper door help");

    showHeader("Door commands");
    for (auto dat : ALL_DOOR_ACTION_TYPES) {
        const int cmdWidth = 6;
        sendToUser(QString("  %1%2 [dir] - executes \"%3 ... [dir]\"\r\n")
                       .arg(prefixChar)
                       .arg(getParserCommandName(dat).describe(), -cmdWidth)
                       .arg(QString{getCommandName(dat)}));
    }

    showDoorVariableHelp();

    showHeader("Destructive commands");
    sendToUser(
        QString("  %1removedoornames   - removes all secret door names from the current map\r\n")
            .arg(prefixChar));
}

void AbstractParser::showDoorVariableHelp()
{
    showHeader("Door variables");
    for (auto dir : ALL_EXITS_NESWUD) {
        auto lower = lowercaseDirection(dir);
        if (lower == nullptr)
            continue;
        auto upper = static_cast<char>(toupper(lower[0]));
        sendToUser(
            QString("  $$DOOR_%1$$   - secret name of door leading %2\r\n").arg(upper).arg(lower));
    }
    sendToUser("  $$DOOR$$     - the same as 'exit'\r\n");
}

void AbstractParser::doMove(const CommandEnum cmd)
{
    // REVISIT: should "look" commands be queued?
    assert(isDirectionNESWUD(cmd) || cmd == CommandEnum::LOOK);
    m_queue.enqueue(cmd);
    pathChanged();
    if (isOffline())
        offlineCharacterMove(cmd);
}

bool AbstractParser::tryParseGenericDoorCommand(const QString &str)
{
    if (!str.contains("$$DOOR"))
        return false;

    const char pattern[] = "$$DOOR_X$$";
    for (auto dir : ALL_EXITS_NESWUD) {
        const auto c = static_cast<char>(std::toupper(Mmapper2Exit::charForDir(dir)));
        auto buf = makeCharBuffer(pattern);
        buf.replaceAll('X', c);
        if (str.contains(buf.getBuffer())) {
            genericDoorCommand(str, dir);
            return true;
        }
    }
    return false;
}

static ExitDirEnum convert_to_ExitDirection(const CommandEnum dir)
{
    if (!isDirectionNESWUD(dir))
        throw std::invalid_argument("dir");
    return getDirection(dir);
}

static CommandEnum convert_to_CommandIdType(const ExitDirEnum dir)
{
    const auto result = static_cast<CommandEnum>(dir);
    if (!isDirectionNESWUD(result))
        throw std::invalid_argument("dir");
    return result;
}

static CommandEnum getRandomDirection()
{
    return convert_to_CommandIdType(chooseRandomElement(ALL_EXITS_NESWUD));
}

void AbstractParser::doOfflineCharacterMove()
{
    if (m_queue.isEmpty()) {
        return;
    }

    const RAIICallback timerRaii{[this]() { this->m_offlineCommandTimer.start(); }};

    CommandEnum direction = m_queue.dequeue();
    if (m_mapData->isEmpty()) {
        sendToUser("Alas, you cannot go that way...\r\n");
        sendPromptToUser();
        return;
    }

    const bool flee = direction == CommandEnum::FLEE;
    const bool scout = direction == CommandEnum::SCOUT;
    static constexpr const bool onlyUseActualExits = true;

    // Note: flee and scout are mutually exclusive.
    if (flee) {
        sendToUser("You flee head over heels.\r\n");
        direction = getRandomDirection(); // pointless if onlyUseActualExits is true
    } else if (scout) {
        direction = m_queue.dequeue();
    }

    const auto rs1 = RoomSelection(*m_mapData, m_mapData->getPosition());
    if (rs1.empty()) {
        sendToUser("Alas, you cannot go that way...\r\n");
        return;
    }

    const Room &here = deref(rs1.getFirstRoom());
    if (direction == CommandEnum::LOOK) {
        sendRoomInfoToUser(&here);
        sendRoomExitsInfoToUser(&here);
        sendPromptToUser(here);
        return;
    }

    const auto getExit = [this, flee, &here, &direction]() -> const Exit & {
        if (flee && onlyUseActualExits) {
            if (auto opt = here.getRandomExit()) {
                direction = convert_to_CommandIdType(opt->dir);
            } else {
                // movement will fail; "PANIC! You couldn't escape!"
            }
        }

        // REVISIT: Should we update the direction hint to the Path Machine?
        const bool updateDir = false;

        const auto &moveDir = here.getExitMaybeRandom(convert_to_ExitDirection(direction));
        const auto actualdir = convert_to_CommandIdType(moveDir.dir);
        const Exit &e = moveDir.exit;

        if (actualdir != direction) {
            sendToUser("You feel confused and move along randomly...\r\n");
            // REVISIT: remove this spam?
            qDebug() << "Randomly moving" << getLowercase(actualdir) << "instead of"
                     << getLowercase(direction);

            if (updateDir)
                direction = actualdir;
        }

        return e;
    };

    const auto showMovement = [this, flee, scout](const CommandEnum direction,
                                                  const Room *const otherRoom) {
        const auto showOtherRoom = [this, otherRoom]() {
            sendRoomInfoToUser(otherRoom);
            sendRoomExitsInfoToUser(otherRoom);
        };

        if (scout) {
            sendToUser(QByteArray("You quietly scout ")
                           .append(getLowercase(direction))
                           .append("wards...\r\n"));
            showOtherRoom();
            sendToUser("\r\nYou stop scouting.\r\n");
            sendPromptToUser();
            return;
        }

        if (flee) {
            // REVISIT: Does MUME actually show you the direction when you flee?
            sendToUser(QByteArray("You flee ").append(getLowercase(direction)).append("."));
        }

        showOtherRoom();
        sendPromptToUser(*otherRoom);

        // Create character move event for main move/search algorithm
        auto ev = ParseEvent::createEvent(direction,
                                          otherRoom->getName(),
                                          otherRoom->getDynamicDescription(),
                                          otherRoom->getStaticDescription(),
                                          ExitsFlagsType{},
                                          PromptFlagsType::fromRoomTerrainType(
                                              otherRoom->getTerrainType()),
                                          ConnectedRoomFlagsType{});
        emit event(SigParseEvent{ev});
        pathChanged();
    };

    const Exit &e = getExit(); /* NOTE: getExit() can modify direction */
    if (e.isExit() && !e.outIsEmpty()) {
        auto rs2 = RoomSelection(*m_mapData);
        if (const Room *const otherRoom = rs2.getRoom(e.outFirst())) {
            showMovement(direction, otherRoom);
            return;
        }
    }

    if (!flee || scout) {
        sendToUser("Alas, you cannot go that way...\r\n");
    } else {
        sendToUser("PANIC! You couldn't escape!\r\n");
    }
    sendPromptToUser(here);
}

void AbstractParser::offlineCharacterMove(const CommandEnum direction)
{
    if (direction == CommandEnum::FLEE) {
        m_queue.enqueue(direction);
    }

    if (!m_offlineCommandTimer.isActive()) {
        m_offlineCommandTimer.start();
    }
}

void AbstractParser::sendRoomInfoToUser(const Room *r)
{
    if (r == nullptr) {
        return;
    }

    const auto &settings = getConfig().parser;
    static constexpr const auto ESCAPE = S_ESC;

    QByteArray roomName("\r\n");
    if (!settings.roomNameColor.isEmpty()) {
        roomName += ESCAPE + settings.roomNameColor.toLatin1();
    }
    roomName += r->getName().toQByteArray() + ESCAPE + "[0m\r\n";
    sendToUser(roomName);

    QByteArray roomDescription;
    if (!r->getStaticDescription().toQString().trimmed().isEmpty()) {
        if (!settings.roomDescColor.isEmpty()) {
            roomDescription += ESCAPE + settings.roomDescColor.toLatin1();
        }
        roomDescription += r->getStaticDescription().toQString().trimmed().toLatin1() + ESCAPE
                           + "[0m";
        roomDescription.replace("\n", "\r\n");
        sendToUser(roomDescription + "\r\n");
    }

    QByteArray dynamicDescription = r->getDynamicDescription().toQString().trimmed().toLatin1();
    if (!dynamicDescription.isEmpty()) {
        dynamicDescription.replace("\n", "\r\n");
        sendToUser(dynamicDescription + "\r\n");
    }
}

void AbstractParser::sendRoomExitsInfoToUser(const Room *const r)
{
    std::ostringstream os;
    sendRoomExitsInfoToUser(os, r);
    sendToUser(::toQByteArrayLatin1(os.str()));
}

void AbstractParser::sendRoomExitsInfoToUser(std::ostream &os, const Room *const r)
{
    if (r == nullptr) {
        return;
    }
    const char sunCharacter = (m_mumeClock->getMumeMoment().toTimeOfDay() <= MumeTimeEnum::DAY)
                                  ? '*'
                                  : '^';
    uint exitCount = 0;
    QString etmp = "Exits/emulated:";
    for (const auto direction : ALL_EXITS_NESWUD) {
        bool door = false;
        bool exit = false;
        bool road = false;
        bool trail = false;
        bool climb = false;
        bool directSun = false;
        bool swim = false;

        const Exit &e = r->exit(direction);
        if (e.isExit()) {
            exitCount++;
            exit = true;
            etmp += " ";

            RoomTerrainEnum sourceTerrain = r->getTerrainType();
            if (!e.outIsEmpty()) {
                const auto targetId = e.outFirst();
                auto rs = RoomSelection(*m_mapData);
                if (const Room *const targetRoom = rs.getRoom(targetId)) {
                    const RoomTerrainEnum targetTerrain = targetRoom->getTerrainType();

                    // Sundeath exit flag modifiers
                    if (targetRoom->getSundeathType() == RoomSundeathEnum::SUNDEATH) {
                        directSun = true;
                        etmp += sunCharacter;
                    }

                    // Terrain type exit modifiers
                    if (targetTerrain == RoomTerrainEnum::RAPIDS
                        || targetTerrain == RoomTerrainEnum::UNDERWATER
                        || targetTerrain == RoomTerrainEnum::WATER) {
                        swim = true;
                        etmp += "~";

                    } else if (targetTerrain == RoomTerrainEnum::ROAD
                               && sourceTerrain == RoomTerrainEnum::ROAD) {
                        road = true;
                        etmp += "=";
                    }
                }
            }

            if (!road && e.getExitFlags().isRoad()) {
                if (sourceTerrain == RoomTerrainEnum::ROAD) {
                    road = true;
                    etmp += "=";
                } else {
                    trail = true;
                    etmp += "-";
                }
            }

            if (e.isDoor()) {
                door = true;
                etmp += "{";
            } else if (e.getExitFlags().isClimb()) {
                climb = true;
                etmp += "|";
            }

            etmp += lowercaseDirection(direction);
        }

        if (door) {
            etmp += "}";
        } else if (climb) {
            etmp += "|";
        }
        if (swim) {
            etmp += "~";
        } else if (road) {
            etmp += "=";
        } else if (trail) {
            etmp += "-";
        }
        if (directSun) {
            etmp += sunCharacter;
        }
        if (exit) {
            etmp += ",";
        }
    }
    if (exitCount == 0) {
        etmp += " none.";
    } else {
        etmp.replace(etmp.length() - 1, 1, '.');
    }

    QByteArray cn = enhanceExits(r);
    os << ::toStdStringLatin1(etmp.toLatin1() + cn);

    if (getConfig().mumeNative.showNotes) {
        const auto &ns = r->getNote();
        if (!ns.isEmpty()) {
            QByteArray note = "Note: " + ns.toQByteArray() + "\r\n";
            os << ::toStdStringLatin1(note);
        }
    }
}

void AbstractParser::sendPromptToUser()
{
    if (m_overrideSendPrompt) {
        m_overrideSendPrompt = false;
        return;
    }

    if (!m_lastPrompt.isEmpty() && isOnline()) {
        if (!m_compactMode) {
            sendToUser("\r\n");
        }
        sendToUser(m_lastPrompt, true);
        return;
    }

    // Emulate prompt mode
    auto rs = RoomSelection(*m_mapData);
    if (const Room *const r = rs.getRoom(
            getNextPosition())) // REVISIT: Should this be the current position?
        sendPromptToUser(*r);
    else
        sendPromptToUser('?', '?');
}

void AbstractParser::sendPromptToUser(const Room &r)
{
    const auto lightType = r.getLightType();
    const auto terrainType = r.getTerrainType();
    sendPromptToUser(lightType, terrainType);
}

void AbstractParser::sendPromptToUser(const RoomLightEnum lightType,
                                      const RoomTerrainEnum terrainType)
{
    char light = getLightSymbol(lightType);
    MumeMoment moment = m_mumeClock->getMumeMoment();
    if (light == 'o' && moment.isMoonBright() && moment.isMoonVisible()) {
        // Moon is out
        light = ')';
    }
    const char terrain = getTerrainSymbol(terrainType);
    sendPromptToUser(light, terrain);
}

void AbstractParser::sendPromptToUser(const char light, const char terrain)
{
    if (!m_compactMode) {
        sendToUser("\r\n");
    }
    QByteArray prompt;
    prompt += light;
    prompt += terrain;
    prompt += ">";
    sendToUser(prompt, true);
}

void AbstractParser::performDoorCommand(const ExitDirEnum direction, const DoorActionEnum action)
{
    Coordinate c = getTailPosition();

    auto cn = getCommandName(action) + " ";
    auto dn = m_mapData->getDoorName(c, direction).toQByteArray();

    bool needdir = false;

    if (direction == ExitDirEnum::UNKNOWN) {
        // If there is only one secret assume that is what needs opening
        auto secretCount = 0;
        for (const auto i : ALL_EXITS_NESWUD) {
            if (m_mapData->getExitFlag(c, i, ExitFieldVariant{DoorFlags{DoorFlagEnum::HIDDEN}})) {
                dn = m_mapData->getDoorName(c, i).toQByteArray();
                secretCount++;
            }
        }
        if (secretCount == 1 && !dn.isEmpty()) {
            needdir = false;
        } else {
            // Otherwise open any exit
            needdir = true;
            dn = "exit";
        }
    } else if (dn.isEmpty()) {
        needdir = true;
        dn = "exit";
    } else {
        // Check if we need to add a direction to the door name
        for (const auto i : ALL_EXITS_NESWUD) {
            if ((i != direction) && (m_mapData->getDoorName(c, i).toQByteArray() == dn)) {
                needdir = true;
            }
        }
    }

    cn += dn;
    if (needdir && isNESWUD(direction)) {
        cn += " ";
        cn += Mmapper2Exit::charForDir(direction);
    }
    cn += "\r\n";

    if (isOnline()) { // online mode
        emit sendToMud(cn);
        sendToUser("--->" + cn);
        m_overrideSendPrompt = true;
    } else {
        sendToUser("--->" + cn);
        sendOkToUser();
    }
}

void AbstractParser::genericDoorCommand(QString command, const ExitDirEnum direction)
{
    QByteArray cn = emptyByteArray;
    Coordinate c = getTailPosition();

    auto dn = m_mapData->getDoorName(c, direction).toQByteArray();

    bool needdir = false;
    if (dn.isEmpty()) {
        dn = "exit";
        needdir = true;
    } else {
        for (const auto i : ALL_EXITS_NESWUD) {
            if ((i != direction) && (m_mapData->getDoorName(c, i).toQByteArray() == dn)) {
                needdir = true;
            }
        }
    }
    if (isNESWUD(direction)) {
        QChar dirChar = Mmapper2Exit::charForDir(direction);
        cn += dn;
        if (needdir) {
            cn += " ";
            cn += dirChar.toLatin1();
        }
        cn += "\r\n";
        command = command.replace(QString("$$DOOR_%1$$").arg(dirChar.toUpper()), cn);
    } else if (direction == ExitDirEnum::UNKNOWN) {
        cn += dn + "\r\n";
        command = command.replace("$$DOOR$$", cn);
    }

    if (isOnline()) { // online mode
        emit sendToMud(command.toLatin1());
        sendToUser("--->" + command);
        m_overrideSendPrompt = true;
    } else {
        sendToUser("--->" + command);
        sendOkToUser();
    }
}

void AbstractParser::setExitFlags(const ExitFlags ef, const ExitDirEnum dir)
{
    m_exitsFlags.set(dir, ef);
}

void AbstractParser::setConnectedRoomFlag(const DirectSunlightEnum light, const ExitDirEnum dir)
{
    m_connectedRoomFlags.setDirectSunlight(dir, light);
}

void AbstractParser::printRoomInfo(const RoomFields fieldset)
{
    if (m_mapData->isEmpty())
        return;

    auto rs = RoomSelection(*m_mapData);
    if (const Room *const r = rs.getRoom(getNextPosition())) {
        // TODO: use QStringBuilder (or std::ostringstream)?
        QString result;

        if (fieldset.contains(RoomFieldEnum::NAME)) {
            result += r->getName().toQByteArray() + "\r\n";
        }
        if (fieldset.contains(RoomFieldEnum::DESC)) {
            result += r->getStaticDescription().toQByteArray();
        }
        if (fieldset.contains(RoomFieldEnum::DYNAMIC_DESC)) {
            result += r->getDynamicDescription().toQByteArray();
        }
        if (fieldset.contains(RoomFieldEnum::NOTE)) {
            result += "Note: " + r->getNote().toQByteArray() + "\r\n";
        }

        sendToUser(result);
    }
}

void AbstractParser::sendGTellToUser(const QString &color, const QString &from, const QString &text)
{
    const QString tell
        = QString("\x1b%1%2 tells you [GT] '%3'\x1b[0m").arg(color).arg(from).arg(text);

    if (m_proxy.isGmcpModuleEnabled(GmcpModuleTypeEnum::MMAPPER_GROUPTELL)) {
        m_proxy.gmcpToUser(GmcpMessage(GmcpMessageTypeEnum::MMAPPER_GROUPTELL,
                                       QString(R"({ "name": "%1", "text": "%2" })")
                                           .arg(GmcpUtils::escapeGmcpStringData(from))
                                           .arg(GmcpUtils::escapeGmcpStringData(tell))));
        return;
    }

    sendToUser("\r\n" + tell.toLatin1() + "\r\n");
    sendPromptToUser();
}

void AbstractParser::printRoomInfo(const RoomFieldEnum field)
{
    printRoomInfo(RoomFields{field});
}

void AbstractParser::eval(const std::string &name,
                          const std::shared_ptr<const syntax::Sublist> &syntax,
                          StringView input)
{
    using namespace syntax;
    const auto thisCommand = std::string(1, prefixChar) + name;
    const auto completeSyntax = buildSyntax(stringToken(thisCommand), syntax);
    sendToUser(processSyntax(completeSyntax, thisCommand, input));
}
