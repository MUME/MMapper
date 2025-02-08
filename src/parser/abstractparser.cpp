// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "abstractparser.h"

#include "../clock/mumeclock.h"
#include "../global/Consts.h"
#include "../global/LineUtils.h"
#include "../global/RAII.h"
#include "../global/StringView.h"
#include "../global/TextUtils.h"
#include "../global/logging.h"
#include "../global/parserutils.h"
#include "../global/progresscounter.h"
#include "../global/random.h"
#include "../global/tests.h"
#include "../map/CommandId.h"
#include "../map/ConnectedRoomFlags.h"
#include "../map/ExitsFlags.h"
#include "../map/PromptFlags.h"
#include "../mapdata/mapdata.h"
#include "../proxy/GmcpUtils.h"
#include "../proxy/proxy.h"
#include "../syntax/Accept.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "Abbrev.h"
#include "AbstractParser-Commands.h"
#include "AbstractParser-Utils.h"
#include "CommandQueue.h"
#include "DoorAction.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
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

using namespace char_consts;

NODISCARD static char getTerrainSymbol(const RoomTerrainEnum type)
{
    switch (type) {
    case RoomTerrainEnum::UNDEFINED:
        return C_SPACE;
    case RoomTerrainEnum::INDOORS:
        return C_OPEN_BRACKET; // [  // indoors
    case RoomTerrainEnum::CITY:
        return C_POUND_SIGN; // #  // city
    case RoomTerrainEnum::FIELD:
        return C_PERIOD; // .  // field
    case RoomTerrainEnum::FOREST:
        return 'f'; // f  // forest
    case RoomTerrainEnum::HILLS:
        return C_OPEN_PARENS; // (  // hills
    case RoomTerrainEnum::MOUNTAINS:
        return C_LESS_THAN; // <  // mountains
    case RoomTerrainEnum::SHALLOW:
        return C_PERCENT_SIGN; // %  // shallow
    case RoomTerrainEnum::WATER:
        return C_TILDE; // ~  // water
    case RoomTerrainEnum::RAPIDS:
        return 'W'; // W  // rapids
    case RoomTerrainEnum::UNDERWATER:
        return 'U'; // U  // underwater
    case RoomTerrainEnum::ROAD:
        return C_PLUS_SIGN; // +  // road
    case RoomTerrainEnum::TUNNEL:
        return C_EQUALS; // =  // tunnel
    case RoomTerrainEnum::CAVERN:
        return 'O'; // O  // cavern
    case RoomTerrainEnum::BRUSH:
        return C_COLON; // :  // brush
    case RoomTerrainEnum::DEATHTRAP:
        return 'X';
    }

    return C_SPACE;
}

NODISCARD static char getLightSymbol(const RoomLightEnum lightType)
{
    switch (lightType) {
    case RoomLightEnum::DARK:
        return 'o';
    case RoomLightEnum::LIT:
    case RoomLightEnum::UNDEFINED:
        return C_ASTERISK;
    }

    return C_QUESTION_MARK;
}

struct NODISCARD Help final
{
    const Abbrev &cmd;
    std::string_view desc;
};

struct NODISCARD Helps final
{
private:
    std::vector<Help> m_helps;
    int m_maxCmdLength = 0;

public:
    explicit Helps(std::vector<Help> helps)
        : m_helps(std::move(helps))
    {
        if (m_helps.empty()) {
            throw std::runtime_error("requires at least one help");
        }

        const auto &max = std::max_element(std::begin(m_helps),
                                           std::end(m_helps),
                                           [](const Help &a, const Help &b) {
                                               return a.cmd.getLength() < b.cmd.getLength();
                                           });
        m_maxCmdLength = max->cmd.getLength();
    }

    NODISCARD std::string format(const char prefixChar) const
    {
        std::ostringstream ss;
        for (const auto &help : m_helps) {
            ss << "  ";
            ss << prefixChar;
            ss << help.cmd.getCommand();
            const int spaces = m_maxCmdLength - help.cmd.getLength();
            for (int i = 0; i < spaces; ++i) {
                ss << char_consts::C_SPACE;
            }
            ss << " - ";
            ss << help.desc;
            ss << char_consts::C_NEWLINE;
        }
        return std::move(ss).str();
    }
};

AbstractParserOutputs::~AbstractParserOutputs() = default;

AbstractParser::AbstractParser(MapData &md,
                               MumeClock &mc,
                               ProxyMudConnectionApi &proxyMudConnection,
                               ProxyUserGmcpApi &proxyUserGmcp,
                               GroupManagerApi &group,
                               QObject *const parent,
                               AbstractParserOutputs &outputs,
                               ParserCommonData &commonData)
    : ParserCommon{parent, mc, md, group, proxyUserGmcp, outputs, commonData}
    , m_proxyMudConnection{proxyMudConnection}
{
    QObject::connect(&m_offlineCommandTimer, &QTimer::timeout, this, [this]() {
        doOfflineCharacterMove();
    });
    QObject::connect(&m_commonData.timers,
                     &CTimers::sig_sendTimersUpdateToUser,
                     this,
                     [this](const std::string &str) { timersUpdate(str); });

    // MUME only attempts up to 4 commands per second (i.e. 250ms)
    m_offlineCommandTimer.setInterval(250);
    m_offlineCommandTimer.setSingleShot(true);

    initSpecialCommandMap();
}

MumeXmlParserBase::~MumeXmlParserBase() = default;
AbstractParser::~AbstractParser() = default;

void MumeXmlParserBase::onReset()
{
    m_commonData.lastPrompt = "";
    getQueue().clear();
}

void MumeXmlParserBase::parseExits(std::ostream &os)
{
    QString str = normalizeStringCopy(m_commonData.exits);
    m_commonData.connectedRoomFlags.reset();
    m_commonData.exitsFlags.reset();
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
        [&doors, &closed, &road, &climb, &portal, &directSun](const char sign) -> bool {
        using namespace char_consts;
        switch (sign) {
        case C_OPEN_PARENS: // open door
            doors = true;
            break;
        case C_OPEN_BRACKET: // closed door
            doors = true;
            closed = true;
            break;
        case C_POUND_SIGN: // broken door
            doors = true;
            break;
        case C_EQUALS: // road
            road = true;
            break;
        case C_MINUS_SIGN: // trail
            road = true;
            break;
        case C_SLASH: // upward climb
            climb = true;
            break;
        case C_BACKSLASH: // downward climb
            climb = true;
            break;
        case C_OPEN_CURLY: // portal
            portal = true;
            break;
        case C_ASTERISK: // sunlit room (troll/orc only)
            directSun = true;
            break;
        case C_CARET: // outdoors room (troll only)
            directSun = true;
            break;
        default:
            return false;
        }
        return true;
    };

    if (str.length() > 5 && str.at(5).toLatin1() != char_consts::C_COLON) {
        // Ainur exits
        static const QRegularExpression rx(
            R"(^\s*([\^\*~\-={#\[\(\\\/]+)?([A-za-z]+)([\^\*~\-=}#\]\)\\\/]+)?\s+\- (.*)$)");
        static const QRegularExpression newline(R"(\r?\n)");
        for (const auto &part : str.split(newline)) {
            const auto match = rx.match(part);
            if (match.hasMatch()) {
                // Parse exit flag
                const auto &signs = match.captured(1);
                for (const QChar qc : signs) {
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
        for (int i = 7; i < length; ++i) {
            const char c = str.at(i).toLatin1();
            if (!parse_exit_flag(c)) {
                switch (c) {
                case C_SPACE: // empty space means reset for next exit
                    reset_exit_flags();
                    break;

                case 'n':
                    if ((i + 2) < length && str.at(i + 2).toLatin1() == 'r') { // north
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
        auto &exitsFlags = m_commonData.exitsFlags;
        auto &connectedRoomFlags = m_commonData.connectedRoomFlags;
        auto &trollExitMapping = m_commonData.trollExitMapping;

        exitsFlags.setValid();

        if (trollExitMapping) {
            connectedRoomFlags.setTrollMode();
        }

        // Orcs and trolls can detect exits with direct sunlight
        const bool foundDirectSunlight = connectedRoomFlags.hasAnyDirectSunlight();
        if (foundDirectSunlight || trollExitMapping) {
            connectedRoomFlags.setValid();
            for (const ExitDirEnum alt_dir : ALL_EXITS_NESWUD) {
                const auto eThisExit = exitsFlags.get(alt_dir);
                const auto eThisClosed = closedDoorFlag.get(alt_dir);

                // Do not flag indirect sunlight if there was a closed door, no exit, or we saw direct sunlight
                if (!eThisExit.isExit() || eThisClosed.isDoor()
                    || connectedRoomFlags.hasDirectSunlight(alt_dir)) {
                    continue;
                }

                // Flag indirect sun
                setConnectedRoomFlag(DirectSunlightEnum::SAW_NO_DIRECT_SUN, alt_dir);
            }
        }
    }

    auto &exits = m_commonData.exits;
    if (const auto &room = m_mapData.findRoomHandle(getNextPosition())) {
        // REVISIT: Why is this a QString in the first place?
        const auto exs = mmqt::toStdStringUtf8(exits);
        StringView sv{exs};
        sv.trimRight();
        os << sv.getStdStringView();

        // REVISIT: Enhanced exits and notes are sourced from Mmapper,
        // so it's misleading if we claim this text came from Mume.
        {
            AnsiOstream aos{os};
            enhanceExits(aos, *room);
        }

        if (getConfig().mumeNative.showNotes) {
            AnsiOstream aos{os};
            displayRoom(aos, *room, RoomFieldFlags{RoomFieldEnum::NOTE});
        }
    } else {
        os << mmqt::toStdStringUtf8(exits);
    }
}

// NOTE: This is now fully redundant, since room sanitizer::sanitizeXXX()
// handles whitespace, removes ansi codes, and converts from Utf8 to ASCII.
//
QString normalizeStringCopy(QString string)
{
    // Remove ANSI first, since we don't want Utf8
    // transliterations to accidentally count as ANSI.
    ParserUtils::removeAnsiMarksInPlace(string);
    mmqt::toAsciiInPlace(string);
    string.remove(char_consts::C_CARRIAGE_RETURN);
    return string;
}

RoomId ParserCommon::getNextPosition() const
{
    const auto id = m_mapData.getCurrentRoomId().value_or(INVALID_ROOMID);
    auto &queue = m_commonData.queue;

    CommandQueue tmpqueue;
    if (!queue.isEmpty()) {
        // Next position in the prespammed path
        tmpqueue.enqueue(queue.head());
    }

    const auto last = m_mapData.getLast(id, tmpqueue);
    return last.value_or(id);
}

RoomId ParserCommon::getTailPosition() const
{
    const auto id = m_mapData.getCurrentRoomId().value_or(INVALID_ROOMID);
    const auto last = m_mapData.getLast(id, m_commonData.queue);
    return last.value_or(id);
}

void ParserCommon::emulateExits(AnsiOstream &os, const CommandEnum move)
{
    const auto nextRoom = [this, &move]() -> RoomId {
        // Use movement direction to find the next coordinate
        if (isDirectionNESWUD(move)) {
            if (const auto &sourceRoom = m_mapData.getCurrentRoom()) {
                const auto &exit = sourceRoom->getExit(getDirection(move));
                if (exit.exitIsExit() && !exit.outIsEmpty()) {
                    const auto &map = m_mapData.getCurrentMap();
                    if (const auto &targetRoom = map.findRoomHandle(exit.outFirst())) {
                        return targetRoom->getId();
                    }
                }
            }
        }
        // Fallback to next position in prespammed path
        return getNextPosition();
    }();

    if (const auto &r = m_mapData.findRoomHandle(nextRoom)) {
        sendRoomExitsInfoToUser(os, r);
    }
}

void AbstractParser::slot_parseNewUserInput(const TelnetData &data)
{
    const auto &line = data.line.getQByteArray();
    auto parse_and_send = [this, &line]() {
        sendToUser(SendToUserSource::NoLongerPrompted, "", true);

        auto parse = [this, &line]() -> bool {
            const QString input = mmqt::getCommand(line);

            try {
                return parseUserCommands(input);
            } catch (const std::exception &ex) {
                qWarning() << "Exception: " << ex.what();
                sendToUser(SendToUserSource::FromMMapper,
                           QString::asprintf("An exception occurred: %s\n", ex.what()));
                sendPromptToUser();
                return false;
            }
        };

        if (parse()) {
            sendToMud(QString::fromUtf8(line));
        }
    };

    switch (data.type) {
    case TelnetDataEnum::Backspace:
    case TelnetDataEnum::Empty:
        qWarning() << "unexpected Backspace or Empty command";
        sendToMud(QString::fromUtf8(line));
        break;
    case TelnetDataEnum::Prompt:
        qWarning() << "prompt sent from user to mud";
        sendToMud(QString::fromUtf8(line));
        break;
    case TelnetDataEnum::CRLF:
    case TelnetDataEnum::LF:
        parse_and_send();
        break;
    }
}

NODISCARD static QString compressDirections(const QString &original)
{
    QString ans;
    int curnum = 0;
    QChar curval = char_consts::C_NUL;
    Coordinate delta;
    const auto addDirs = [&ans, &curnum, &curval, &delta]() {
        assert(curnum >= 1);
        assert(curval != char_consts::C_NUL);
        if (curnum > 1) {
            ans.append(QString::number(curnum));
        }
        ans.append(curval);

        const auto dir = Mmapper2Exit::dirForChar(curval.toLatin1());
        delta += ::exitDir(dir) * curnum;
    };

    for (const QChar c : original) {
        if (curnum != 0 && curval == c) {
            ++curnum;
        } else {
            if (curnum != 0) {
                addDirs();
            }
            curnum = 1;
            curval = c;
        }
    }
    if (curnum != 0) {
        addDirs();
    }

    bool wantDelta = true;
    if (wantDelta) {
        auto addNumber =
            [&curnum, &curval, &addDirs, &ans](const int n, const char pos, const char neg) {
                if (n == 0) {
                    return;
                }
                curnum = std::abs(n);
                curval = (n < 0) ? neg : pos;
                ans += char_consts::C_SPACE;
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

class NODISCARD ShortestPathEmitter final : public ShortestPathRecipient
{
private:
    AbstractParser &parser;

public:
    explicit ShortestPathEmitter(AbstractParser &_parser)
        : parser(_parser)
    {}
    ~ShortestPathEmitter() final;

private:
    void virt_receiveShortestPath(QVector<SPNode> spnodes, int endpoint) final
    {
        const SPNode *spnode = &spnodes[endpoint];
        auto name = spnode->r->getName();
        parser.sendToUser(SendToUserSource::FromMMapper,
                          "Distance " + std::to_string(spnode->dist) + ": " + name.toStdStringUtf8()
                              + "\n");
        QString dirs;
        while (spnode->parent >= 0) {
            if (&spnodes[spnode->parent] == spnode) {
                parser.sendToUser(SendToUserSource::FromMMapper, "ERROR: loop\n");
                break;
            }
            dirs.append(Mmapper2Exit::charForDir(spnode->lastdir));
            spnode = &spnodes[spnode->parent];
        }
        std::reverse(dirs.begin(), dirs.end());
        parser.sendToUser(SendToUserSource::FromMMapper, "dirs: " + compressDirections(dirs) + "\n");
    }
};

ShortestPathEmitter::~ShortestPathEmitter() = default;

void AbstractParser::searchCommand(const RoomFilter &f)
{
    if (f.patternKind() == PatternKindsEnum::NONE) {
        onNewRoomSelection(SigRoomSelection{});
        sendToUser(SendToUserSource::FromMMapper, "Rooms unselected.\n");
        return;
    }

    auto &map = m_mapData;
    auto found = map.genericFind(f);
    sendToUser(SendToUserSource::FromMMapper,
               QString("%1 room%2 found.\n").arg(found.size()).arg((found.size() == 1) ? "" : "s"));
    const auto tmpSel = RoomSelection::createSelection(std::move(found));
    onNewRoomSelection(SigRoomSelection{tmpSel});
}

void AbstractParser::dirsCommand(const RoomFilter &f)
{
    ShortestPathEmitter sp_emitter(*this);
    if (const auto &r = m_mapData.findRoomHandle(getTailPosition())) {
        MapData::shortestPathSearch(deref(r), sp_emitter, f, 10, 0);
    }
}

ExitDirEnum AbstractParser::tryGetDir(StringView &view)
{
    if (view.isEmpty()) {
        return ExitDirEnum::UNKNOWN;
    }

    auto word = view.takeFirstWord();
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        auto lower = lowercaseDirection(dir);
        if (lower != nullptr && Abbrev{lower, 1}.matches(word)) {
            return dir;
        }
    }

    sendToUser(SendToUserSource::FromMMapper,
               QString("Unexpected direction: \"%1\"\n").arg(word.toQString()));
    throw std::runtime_error("bad direction");
}

void AbstractParser::showCommandPrefix()
{
    using namespace char_consts;
    const auto prefixChar = getPrefixChar();
    const auto quote = static_cast<char>((prefixChar == C_SQUOTE) ? C_DQUOTE : C_SQUOTE);
    sendToUser(SendToUserSource::FromMMapper,
               QString("The current command prefix is: %1%2%1 (e.g. %2help)\n")
                   .arg(quote)
                   .arg(prefixChar));
}

bool AbstractParser::setCommandPrefix(const char prefix)
{
    if (!isValidPrefix(prefix)) {
        return false;
    }
    setConfig().parser.prefixChar = prefix;
    showCommandPrefix();
    return true;
}

// TODO: use something safer than a raw C string
void AbstractParser::showSyntax(const char *rest)
{
    assert(rest != nullptr);
    sendToUser(SendToUserSource::FromMMapper,
               QString::asprintf("Usage: %c%s\n", getPrefixChar(), (rest != nullptr) ? rest : ""));
}

void AbstractParser::doSearchCommand(const StringView view)
{
    if (std::optional<RoomFilter> optFilter = RoomFilter::parseRoomFilter(view.getStdStringView())) {
        searchCommand(optFilter.value());
    } else {
        sendToUser(SendToUserSource::FromMMapper, RoomFilter::parse_help);
    }
}

void AbstractParser::doGetDirectionsCommand(const StringView view)
{
    if (std::optional<RoomFilter> optFilter = RoomFilter::parseRoomFilter(view.getStdStringView())) {
        dirsCommand(optFilter.value());
    } else {
        sendToUser(SendToUserSource::FromMMapper, RoomFilter::parse_help);
    }
}

void AbstractParser::doRemoveDoorNamesCommand()
{
    ProgressCounter dummyPc;
    m_mapData.removeDoorNames(dummyPc);
    sendToUser(SendToUserSource::FromMMapper, "Secret exits purged.\n");
}

void AbstractParser::doGenerateBaseMap()
{
    sendToUser(SendToUserSource::FromMMapper, "Attempting to build the base map...\n");
    emit m_mapData.sig_generateBaseMap();
}

void AbstractParser::doBackCommand()
{
    getQueue().clear();
    sendOkToUser();
    pathChanged();
}

bool AbstractParser::isConnected()
{
    return m_proxyMudConnection.isConnected();
}

void AbstractParser::doConnectToHost()
{
    m_proxyMudConnection.connectToMud();
}

void AbstractParser::doDisconnectFromHost()
{
    m_proxyMudConnection.disconnectFromMud();
}

void AbstractParser::openVoteURL()
{
    QDesktopServices::openUrl(QUrl(
        "https://www.mudconnect.com/cgi-bin/search.cgi?mode=mud_listing&mud=MUME+-+Multi+Users+In+Middle+Earth"));
    sendToUser(SendToUserSource::FromMMapper, "Thank you kindly for voting!\n");
}

void AbstractParser::showHelpCommands(const bool showAbbreviations)
{
    auto &map = this->m_specialCommandMap;

    struct NODISCARD record final
    {
        std::string from;
        std::string to;
    };

    std::vector<record> records;
    for (const auto &kv : map) {
        const auto &from = kv.first;
        const auto &to = kv.second.fullCommand;
        if (from.empty() || to.empty()) {
            throw std::runtime_error("internal havoc");
        }

        if (showAbbreviations || from == to) {
            records.emplace_back(record{from, to});
        }
    }

    if (records.empty()) {
        return;
    }

    std::sort(std::begin(records), std::end(records), [](const record &a, const record &b) {
        return a.from < b.from;
    });

    const auto prefixChar = getPrefixChar();
    char currentLetter = records[0].from[0];
    for (const auto &rec : records) {
        const auto thisLetter = rec.from[0];
        if (thisLetter != currentLetter) {
            currentLetter = thisLetter;
            sendToUser(SendToUserSource::FromMMapper, "\n");
        }

        if (rec.from == rec.to) {
            sendToUser(SendToUserSource::FromMMapper,
                       QString::asprintf("  %c%s\n", prefixChar, rec.from.c_str()));
        } else {
            sendToUser(SendToUserSource::FromMMapper,
                       QString::asprintf("  %c%-20s -> %c%s\n",
                                         prefixChar,
                                         rec.from.c_str(),
                                         prefixChar,
                                         rec.to.c_str()));
        }
    }
}

void AbstractParser::showHeader(const QString &s)
{
    // TODO: concatenate this another way
    QString result;
    result += "\n";
    result += s;
    result += "\n";
    result += QString(s.size(), C_MINUS_SIGN);
    result += "\n";
    sendToUser(SendToUserSource::FromMMapper, result);
}

void AbstractParser::showMiscHelp()
{
    static const Helps helps = []() -> Helps {
        std::vector<Help> h;
        h.emplace_back(Help{cmdBack, "delete prespammed commands from queue"});
        h.emplace_back(Help{cmdMap, "modify or display stats about the map"});
        h.emplace_back(Help{cmdTime, "display current MUME time"});
        h.emplace_back(Help{cmdVote, "vote for MUME on TMC!"});
        return Helps{std::move(h)};
    }();

    showHeader("Miscellaneous commands");
    sendToUser(SendToUserSource::FromMMapper, helps.format(getPrefixChar()));
}

void AbstractParser::showHelp()
{
    auto s = QString("\n"
                     "MMapper help:\n"
                     "-------------\n"
                     "\n"
                     "Mapping commands:\n"
                     "  %1room        - (see \"%1room ??\" for syntax help)\n"
                     "  %1mark        - (see \"%1mark ??\" for syntax help)\n"
                     "\n"
                     "Utility commands:\n"
                     "  %1gtell       - send group tell\n"
                     "  %1group       - (see \"%1group ??\" for syntax help)\n"
                     "  %1timer       - (see \"%1timer ??\" for syntax help)\n"
                     "\n"
                     "Config commands:\n"
                     "  %1config      - (see \"%1config ??\" for syntax help)\n"
                     "\n"
                     "Help commands:\n"
                     "  %1help      - this help text\n"
                     "  %1help ??   - full syntax help for the help command\n"
                     "  %1doorhelp  - help for door console commands\n"
                     "\n"
                     "Other commands:\n"
                     "  %1dirs [-options] pattern   - directions to matching rooms\n"
                     "  %1search [-options] pattern - select matching rooms\n"
                     "  %1set [prefix [punct-char]] - change command prefix\n"
                     "  %1connect                   - connect to the MUD\n"
                     "  %1disconnect                - disconnect from the MUD\n");

    sendToUser(SendToUserSource::FromMMapper, s.arg(getPrefixChar()));
}

void AbstractParser::showMumeTime()
{
    MumeClock &clock = m_mumeClock;
    const MumeMoment moment = clock.getMumeMoment();
    auto data = clock.toMumeTime(moment) + "\n";
    const MumeClockPrecisionEnum precision = clock.getPrecision();
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
        data += " for " + clock.toCountdown(moment) + " more ticks.\n";

        // Moon data
        data += moment.toMumeMoonTime() + "\n";
        if (precision == MumeClockPrecisionEnum::MINUTE) {
            data += "The moon ";
            if (moment.moonPhase() == MumeMoonPhaseEnum::NEW_MOON) {
                data += "will change phases in";
            } else if (moment.isMoonBelowHorizon()) {
                data += "will rise in";
            } else {
                data += "will set in";
            }
            data += " " + moment.toMoonVisibilityCountDown() + " more ticks.\n";
        }
    }
    sendToUser(SendToUserSource::FromMMapper, data);
}

void AbstractParser::showDoorCommandHelp()
{
    showHeader("MMapper door help");

    showHeader("Door commands");
    for (const DoorActionEnum dat : ALL_DOOR_ACTION_TYPES) {
        const int cmdWidth = 6;
        sendToUser(SendToUserSource::FromMMapper,
                   QString("  %1%2 [dir] - executes \"%3 ... [dir]\"\n")
                       .arg(getPrefixChar())
                       .arg(getParserCommandName(dat).describe(), -cmdWidth)
                       .arg(QString{getCommandName(dat)}));
    }
}

void AbstractParser::doMove(const CommandEnum cmd)
{
    // REVISIT: should "look" commands be queued?
    assert(isDirectionNESWUD(cmd) || cmd == CommandEnum::LOOK);
    getQueue().enqueue(cmd);
    pathChanged();
    if (isOffline()) {
        offlineCharacterMove(cmd);
    }
}

NODISCARD static ExitDirEnum convert_to_ExitDirection(const CommandEnum dir)
{
    if (!isDirectionNESWUD(dir)) {
        throw std::invalid_argument("dir");
    }
    return getDirection(dir);
}

NODISCARD static CommandEnum convert_to_CommandIdType(const ExitDirEnum dir)
{
    const auto result = static_cast<CommandEnum>(dir);
    if (!isDirectionNESWUD(result)) {
        throw std::invalid_argument("dir");
    }
    return result;
}

NODISCARD static CommandEnum getRandomDirection()
{
    return convert_to_CommandIdType(chooseRandomElement(ALL_EXITS_NESWUD));
}

struct NODISCARD ExitDir final
{
    ExitDirEnum dir = ExitDirEnum::NONE;
    const RawExit *exit = nullptr;

    NODISCARD explicit operator bool() const { return exit != nullptr; }
};

NODISCARD static ExitDir getRandomExit(const RoomHandle &r)
{
    // Pick an alternative direction to randomly wander into
    const auto outExits = r.computeExitDirections();
    if (outExits.empty()) {
        return {};
    }

    const auto randomDir = chooseRandomElement(outExits);
    return ExitDir{randomDir, std::addressof(r.getExit(randomDir))};
}

NODISCARD static ExitDir getExitMaybeRandom(const RoomHandle &r, const ExitDirEnum dir)
{
    // REVISIT: The whole room (not just exits) can be flagged as random in MUME.
    const auto &e = r.getExit(dir);

    if (e.exitIsRandom()) {
        if (const ExitDir exitDir = getRandomExit(r)) {
            return exitDir;
        }
    }

    return ExitDir{dir, std::addressof(e)};
}

void AbstractParser::doOfflineCharacterMove()
{
    auto &queue = getQueue();
    if (queue.isEmpty()) {
        return;
    }

    const RAIICallback timerRaii{[this]() { this->m_offlineCommandTimer.start(); }};

    CommandEnum direction = queue.dequeue();
    if (m_mapData.isEmpty()) {
        sendToUser(SendToUserSource::FromMMapper, "Alas, you cannot go that way...\n");
        sendPromptToUser();
        return;
    }

    const bool flee = direction == CommandEnum::FLEE;
    const bool scout = direction == CommandEnum::SCOUT;
    static constexpr const bool onlyUseActualExits = true;

    // Note: flee and scout are mutually exclusive.
    if (flee) {
        sendToUser(SendToUserSource::FromMMapper, "You flee head over heels.\n");
        direction = getRandomDirection(); // pointless if onlyUseActualExits is true
    } else if (scout) {
        direction = queue.dequeue();
    }

    const auto pRoom = m_mapData.getCurrentRoom();
    if (pRoom == std::nullopt) {
        sendToUser(SendToUserSource::FromMMapper, "Alas, you cannot go that way...\n");
        return;
    }

    const auto &here = deref(pRoom);
    if (direction == CommandEnum::LOOK) {
        sendRoomInfoToUser(here);
        sendRoomExitsInfoToUser(here);
        sendPromptToUser(here);
        return;
    }

    const auto getExit = [this, flee, &here, &direction]() -> const RawExit & {
        if (flee && onlyUseActualExits) {
            if (auto opt = getRandomExit(here)) {
                direction = convert_to_CommandIdType(opt.dir);
            } else {
                // movement will fail; "PANIC! You couldn't escape!"
            }
        }

        // REVISIT: Should we update the direction hint to the Path Machine?
        const bool updateDir = false;

        const auto &moveDir = getExitMaybeRandom(here, convert_to_ExitDirection(direction));
        const auto actualdir = convert_to_CommandIdType(moveDir.dir);
        const auto &e = moveDir.exit;

        if (actualdir != direction) {
            sendToUser(SendToUserSource::FromMMapper,
                       "You feel confused and move along randomly...\n");
            // REVISIT: remove this spam?
            qDebug() << "Randomly moving" << getLowercase(actualdir) << "instead of"
                     << getLowercase(direction);

            if (updateDir) {
                direction = actualdir;
            }
        }

        return deref(e);
    };

    const auto showMovement = [this, flee, scout](const CommandEnum dir, const RoomPtr &otherRoom) {
        const auto showOtherRoom = [this, otherRoom]() {
            sendRoomInfoToUser(otherRoom);
            sendRoomExitsInfoToUser(otherRoom);
        };

        if (scout) {
            sendToUser(SendToUserSource::SimulatedOutput,
                       QByteArray("You quietly scout ")
                           .append(getLowercase(dir))
                           .append("wards...\n"));
            showOtherRoom();
            sendToUser(SendToUserSource::SimulatedOutput, "\nYou stop scouting.\n");
            sendPromptToUser();
            return;
        }

        if (flee) {
            // REVISIT: Does MUME actually show you the direction when you flee?
            sendToUser(SendToUserSource::SimulatedOutput,
                       QByteArray("You flee ").append(getLowercase(dir)).append("."));
        }

        showOtherRoom();
        sendPromptToUser(*otherRoom);

        // Create character move event for main move/search algorithm

        auto ev = ParseEvent::createSharedEvent(dir,
                                                otherRoom->getServerId(),
                                                otherRoom->getName(),
                                                otherRoom->getDescription(),
                                                otherRoom->getContents(),
                                                ServerExitIds{},
                                                otherRoom->getTerrainType(),
                                                ExitsFlagsType{},
                                                PromptFlagsType{},
                                                ConnectedRoomFlagsType{});
        onHandleParseEvent(SigParseEvent{ev});
        pathChanged();
    };

    const auto &e = getExit(); /* NOTE: getExit() can modify direction */
    if (e.exitIsExit() && !e.outIsEmpty()) {
        if (const auto &otherRoom = m_mapData.findRoomHandle(e.outFirst())) {
            showMovement(direction, otherRoom);
            return;
        }
    }

    if (!flee || scout) {
        sendToUser(SendToUserSource::FromMMapper, "Alas, you cannot go that way...\n");
    } else {
        sendToUser(SendToUserSource::FromMMapper, "PANIC! You couldn't escape!\n");
    }
    sendPromptToUser(here);
}

void AbstractParser::offlineCharacterMove(const CommandEnum direction)
{
    if (direction == CommandEnum::FLEE) {
        getQueue().enqueue(direction);
    }

    if (!m_offlineCommandTimer.isActive()) {
        m_offlineCommandTimer.start();
    }
}

void AbstractParser::sendRoomInfoToUser(const RoomPtr &r)
{
    if (r == std::nullopt || !r->exists()) {
        return;
    }

    std::ostringstream os;
    {
        AnsiOstream aos{os};
        displayRoom(aos, *r, RoomFieldEnum::NAME | RoomFieldEnum::DESC | RoomFieldEnum::CONTENTS);
    }
    sendToUser(SendToUserSource::SimulatedOutput, std::move(os).str());
}

void ParserCommon::sendRoomExitsInfoToUser(const RoomPtr &r)
{
    if (r == std::nullopt || !r->exists()) {
        return;
    }

    std::ostringstream os;
    {
        AnsiOstream aos{os};
        sendRoomExitsInfoToUser(aos, r);
    }
    sendToUser(SendToUserSource::SimulatedOutput, std::move(os).str());
}

void ParserCommon::sendRoomExitsInfoToUser(AnsiOstream &os, const RoomPtr &r)
{
    if (r == std::nullopt || !r->exists()) {
        return;
    }

    const char sunCharacter = (m_mumeClock.getMumeMoment().toTimeOfDay() <= MumeTimeEnum::DAY)
                                  ? char_consts::C_ASTERISK
                                  : char_consts::C_CARET;

    displayExits(os, *r, sunCharacter);

    if (getConfig().mumeNative.showNotes) {
        displayRoom(os, *r, RoomFieldFlags{RoomFieldEnum::NOTE});
    }
}

void ParserCommon::sendPromptToUser()
{
    auto &overrideSendPrompt = m_commonData.overrideSendPrompt;
    if (overrideSendPrompt) {
        overrideSendPrompt = false;
        return;
    }

    auto &lastPrompt = m_commonData.lastPrompt;
    if (!lastPrompt.isEmpty() && isOnline()) {
        // REVISIT: Technically this is from MUME, but it's being duplicated here.
        sendToUser(SendToUserSource::DuplicatePrompt, lastPrompt, true);
        return;
    }

    // Emulate prompt mode
    if (const auto &r = m_mapData.findRoomHandle(
            getNextPosition())) { // REVISIT: Should this be the current position?
        sendPromptToUser(*r);
    } else {
        sendPromptToUser(C_QUESTION_MARK, C_QUESTION_MARK);
    }
}

void ParserCommon::sendPromptToUser(const RoomHandle &r)
{
    const auto lightType = r.getLightType();
    const auto terrainType = r.getTerrainType();
    sendPromptToUser(lightType, terrainType);
}

void ParserCommon::sendPromptToUser(const RoomLightEnum lightType, const RoomTerrainEnum terrainType)
{
    const char light = m_mumeClock.getMumeMoment().moonVisibility()
                               == MumeMoonVisibilityEnum::BRIGHT
                           ? C_CLOSE_PARENS // Moon is out
                           : getLightSymbol(lightType);
    const char terrain = getTerrainSymbol(terrainType);
    sendPromptToUser(light, terrain);
}

void ParserCommon::sendPromptToUser(const char light, const char terrain)
{
    QByteArray prompt;
    prompt += light;
    prompt += terrain;
    prompt += ">";
    sendToUser(SendToUserSource::SimulatedPrompt, prompt, true);
}

void AbstractParser::executeMudCommand(const QString &command)
{
    sendToUser(SendToUserSource::FromMMapper, "[" + command + "]\n");

    if (isOnline()) {
        sendToMud(command + "\n");
        m_commonData.overrideSendPrompt = true;
    } else {
        sendOkToUser();
    }
}

// TODO: remove conversions to Qt types in this function
void AbstractParser::performDoorCommand(const ExitDirEnum direction, const DoorActionEnum action)
{
    const auto id = getTailPosition();
    auto room = m_mapData.getRoomHandle(id);

    QString command = getCommandName(action) + " exit";
    if (isNESWUD(direction)) {
        command += " ";
        command += Mmapper2Exit::charForDir(direction);
    }
    executeMudCommand(command);
}

void ParserCommon::setExitFlags(const ExitFlags ef, const ExitDirEnum dir)
{
    m_commonData.exitsFlags.set(dir, ef);
}

void ParserCommon::setConnectedRoomFlag(const DirectSunlightEnum light, const ExitDirEnum dir)
{
    m_commonData.connectedRoomFlags.setDirectSunlight(dir, light);
}

void ParserCommon::sendGTellToUser(const QString &color, const QString &from, const QString &text)
{
    const QString tell = QString("\x1B%1%2 tells you [GT] '%3'\x1B[0m").arg(color, from, text);

    if (m_proxyUserGmcp.isUserGmcpModuleEnabled(GmcpModuleTypeEnum::MMAPPER_COMM)) {
        m_proxyUserGmcp.gmcpToUser(
            GmcpMessage(GmcpMessageTypeEnum::MMAPPER_COMM_GROUPTELL,
                        GmcpJson{QString(R"({ "name": "%1", "text": "%2" })")
                                     .arg(GmcpUtils::escapeGmcpStringData(from),
                                          GmcpUtils::escapeGmcpStringData(tell))}));
        return;
    }

    sendToUser(SendToUserSource::GroupTellOutput, "\n" + tell + "\n");
    sendPromptToUser();
}

void AbstractParser::eval(const std::string_view name,
                          const std::shared_ptr<const syntax::Sublist> &syntax,
                          const StringView input)
{
    using namespace syntax;
    const auto thisCommand = std::string(1, getPrefixChar()).append(name);
    const auto completeSyntax = buildSyntax(stringToken(thisCommand), syntax);

    // TODO: output directly to user's ostream instead of returning a string.
    auto result = processSyntax(completeSyntax, thisCommand, input);
    sendToUser(SendToUserSource::FromMMapper, result);
}

void AbstractParser::timersUpdate(const std::string &text)
{
    sendToUser(SendToUserSource::FromMMapper, text);
    sendPromptToUser();
}

void ParserCommon::clearQueue()
{
    auto &queue = getQueue();
    if (queue.isEmpty()) {
        return;
    }

    queue.clear();
    pathChanged();
}

void MumeXmlParserBase::onForcedPositionChange()
{
    MMLOG() << __FUNCTION__ << " called.";
    clearQueue();
}
