// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "mumexmlparser.h"

#include "../global/Consts.h"
#include "../global/parserutils.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"
#include "abstractparser.h"
#include "patterns.h"

#include <cctype>
#include <list>
#include <optional>
#include <sstream>
#include <utility>

#include <QByteArray>
#include <QString>

using namespace char_consts;

class MapData;

namespace { // anonymous
#if defined(XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE)
static constexpr const auto XPS_DEBUG_TO_FILE = static_cast<bool>(
    XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE);
#else
static constexpr const auto XPS_DEBUG_TO_FILE = false;
#endif
} // namespace

static const QByteArray greaterThanChar(">");
static const QByteArray lessThanChar("<");
static const QByteArray greaterThanTemplate("&gt;");
static const QByteArray lessThanTemplate("&lt;");
static const QByteArray ampersand("&");
static const QByteArray ampersandTemplate("&amp;");

MumeXmlParser::MumeXmlParser(MapData &md,
                             MumeClock &mc,
                             ProxyParserApi proxy,
                             GroupManagerApi group,
                             CTimers &timers,
                             QObject *parent)
    : AbstractParser(md, mc, proxy, group, timers, parent)
{
    if (XPS_DEBUG_TO_FILE) {
        QString fileName = "xmlparser_debug.dat";

        file = new QFile(fileName);

        if (!file->open(QFile::WriteOnly))
            return;

        debugStream = new QDataStream(file);
    }
}

MumeXmlParser::~MumeXmlParser()
{
    if (XPS_DEBUG_TO_FILE) {
        file->close();
    }
}

void MumeXmlParser::slot_parseNewMudInput(const TelnetData &data)
{
    switch (data.type) {
    case TelnetDataEnum::DELAY: // Twiddlers
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "DELAY";
            (*debugStream) << "***ETYPE***";
        }
        m_lastPrompt = data.line;
        if (getConfig().parser.removeXmlTags)
            stripXmlEntities(m_lastPrompt);
        parse(data, true);
        break;
    case TelnetDataEnum::PROMPT:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "PROMPT";
            (*debugStream) << "***ETYPE***";
        }
        parse(data, true);
        break;
    case TelnetDataEnum::UNKNOWN:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "OTHER";
            (*debugStream) << "***ETYPE***";
        }
        parse(data, false);
        break;

    case TelnetDataEnum::LF:
    case TelnetDataEnum::CRLF:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "CRLF";
            (*debugStream) << "***ETYPE***";
        }
        // XML and prompts
        parse(data, false);
        break;
    }
    if (XPS_DEBUG_TO_FILE) {
        (*debugStream) << "***S***";
        (*debugStream) << data.line;
        (*debugStream) << "***E***";
    }
}

void MumeXmlParser::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (msg.isCharStatusVars() && msg.getJsonDocument().has_value()
        && msg.getJsonDocument()->isObject()) {
        // "Char.StatusVars {\"race\":\"Troll\",\"subrace\":\"Cave Troll\"}"
        const auto &obj = msg.getJsonDocument()->object();
        const auto &race = obj.value("race");
        if (race.isString()) {
            m_trollExitMapping = (race.toString().compare("Troll", Qt::CaseInsensitive) == 0);
            log("Parser",
                QString("%1 troll exit mapping").arg(m_trollExitMapping ? "Enabling" : "Disabling"));
        }
    }
}

void MumeXmlParser::parse(const TelnetData &data, const bool isGoAhead)
{
    const QByteArray &line = data.line;
    m_lineToUser.clear();
    m_lineFlags.remove(LineFlagEnum::NONE);
    if (!m_lineFlags.isSnoop())
        m_snoopChar.reset();

    for (const char c : line) {
        if (m_readingTag) {
            if (c == C_GREATER_THAN) {
                // send tag
                if (!m_tempTag.isEmpty()) {
                    MAYBE_UNUSED const auto ignored = //
                        element(m_tempTag);
                }

                m_tempTag.clear();

                m_readingTag = false;
                continue;
            }
            m_tempTag.append(c);

        } else {
            if (c == C_LESS_THAN) {
                m_lineToUser.append(characters(m_tempCharacters));
                m_tempCharacters.clear();

                m_readingTag = true;
                continue;
            }
            m_tempCharacters.append(c);
        }
    }

    if (!m_readingTag) {
        m_lineToUser.append(characters(m_tempCharacters));
        m_tempCharacters.clear();
    }
    if (!m_lineToUser.isEmpty()) {
        sendToUser(m_lineToUser, isGoAhead);

        // Simplify the output and run actions
        QByteArray temp = m_lineToUser;
        if (!getConfig().parser.removeXmlTags) {
            stripXmlEntities(temp);
        }
        QString tempStr = temp;
        tempStr = normalizeStringCopy(tempStr.trimmed());
        if (m_snoopChar.has_value() && tempStr.length() > 3 && tempStr.at(0) == C_AMPERSAND
            && tempStr.at(1) == m_snoopChar.value() && tempStr.at(2) == C_SPACE) {
            // Remove snoop prefix (i.e. "&J Exits: north.")
            tempStr = tempStr.mid(3);
        }
        parseMudCommands(tempStr);
    }
}

bool MumeXmlParser::element(const QByteArray &line)
{
    const int length = line.length();

    // REVISIT: Merge this logic with the state machine in parse()
    const auto attributes = [&line]() {
        std::list<std::pair<std::string, std::string>> attributes;

        std::ostringstream os;
        std::optional<std::string> key;

        XmlAttributeStateEnum state = XmlAttributeStateEnum::ELEMENT;
        const auto makeAttribute = [&key, &os, &attributes, &state]() {
            assert(key.has_value());
            // REVISIT: Translate XML entities into text
            attributes.emplace_back(make_pair(key.value(), os.str()));
            key.reset();
            os.str(std::string());
            state = XmlAttributeStateEnum::ATTRIBUTE;
        };
        for (const char c : line) {
            switch (state) {
            case XmlAttributeStateEnum::ELEMENT:
                if (isSpace(c))
                    state = XmlAttributeStateEnum::ATTRIBUTE;
                else
                    continue;
                break;
            case XmlAttributeStateEnum::ATTRIBUTE:
                if (isSpace(c))
                    continue;
                else if (c == C_EQUALS) {
                    key = os.str();
                    os.str(std::string());
                    state = XmlAttributeStateEnum::EQUALS;
                } else
                    os << c;
                break;
            case XmlAttributeStateEnum::EQUALS:
                if (isSpace(c))
                    continue;
                else if (c == C_SQUOTE)
                    state = XmlAttributeStateEnum::SINGLE_QUOTED_VALUE;
                else if (c == C_DQUOTE)
                    state = XmlAttributeStateEnum::DOUBLE_QUOTED_VALUE;
                else {
                    os << c;
                    state = XmlAttributeStateEnum::UNQUOTED_VALUE;
                }
                break;
            case XmlAttributeStateEnum::UNQUOTED_VALUE:
                // Note: This format is not valid according to the W3C XML standard
                if (isSpace(c) || c == C_SLASH)
                    makeAttribute();
                else
                    os << c;
                break;
            case XmlAttributeStateEnum::SINGLE_QUOTED_VALUE:
                if (c == C_SQUOTE)
                    makeAttribute();
                else
                    os << c;
                break;
            case XmlAttributeStateEnum::DOUBLE_QUOTED_VALUE:
                if (c == C_DQUOTE)
                    makeAttribute();
                else
                    os << c;
                break;
            }
        }
        if (key.has_value())
            makeAttribute();
        return attributes;
    }();

    switch (m_xmlMode) {
    case XmlModeEnum::NONE:
        if (length > 0) {
            switch (line.at(0)) {
            case C_SLASH:
                if (line.startsWith("/snoop")) {
                    m_descriptionReady = false;
                    m_lineFlags.remove(LineFlagEnum::SNOOP);
                    if (m_descriptionReady) {
                        if (!m_exitsReady && getConfig().mumeNative.emulatedExits) {
                            m_exitsReady = true;
                            std::ostringstream os;
                            emulateExits(os, m_move);
                            sendToUser(mmqt::toQByteArrayLatin1(snoopToUser(os.str())));
                        }
                        m_promptFlags.reset(); // Don't trust god prompts
                        if (!m_queue.isEmpty() && m_move != CommandEnum::LOOK) // Remove follows
                        {
                            MAYBE_UNUSED const auto ignored = //
                                m_queue.dequeue();
                        }
                        if (m_move != CommandEnum::LOOK)
                            m_queue.enqueue(m_move);
                        move();
                    }

                } else if (line.startsWith("/status")) {
                    m_lineFlags.remove(LineFlagEnum::STATUS);

                } else if (line.startsWith("/weather")) {
                    m_lineFlags.remove(LineFlagEnum::WEATHER);
                    // Certain weather events happen on ticks

                } else if (line.startsWith("/xml")) {
                    sendToUser("[MMapper] Mapper cannot function without XML mode\n");
                    m_queue.clear();
                    m_lineFlags.clear();
                }
                break;
            case 'p':
                if (line.startsWith("prompt")) {
                    m_xmlMode = XmlModeEnum::PROMPT;
                    m_lineFlags.insert(LineFlagEnum::PROMPT);
                    m_lastPrompt = emptyByteArray;
                }
                break;
            case 'e':
                if (line.startsWith("exits")) {
                    m_exits = nullString; // Reset string since payload can be from the 'exit' command
                    m_xmlMode = XmlModeEnum::EXITS;
                    m_lineFlags.insert(LineFlagEnum::EXITS);
                }
                break;
            case 'r':
                if (line.startsWith("room")) {
                    m_xmlMode = XmlModeEnum::ROOM;
                    m_roomName = RoomName{}; // 'name' tag will not show up when blinded
                    m_descriptionReady = false;
                    m_exitsReady = false;
                    m_roomDesc.reset();
                    m_roomContents.reset();
                    m_terrain = RoomTerrainEnum::UNDEFINED;
                    m_exits = nullString;
                    m_promptFlags.reset();
                    m_exitsFlags.reset();
                    m_connectedRoomFlags.reset();
                    m_lineFlags.insert(LineFlagEnum::ROOM);

                    for (const auto &pair : attributes) {
                        if (pair.first.empty() || pair.second.empty())
                            continue;
                        switch (pair.first.at(0)) {
                        case 't':
                            if (pair.first == "terrain") {
                                switch (pair.second.at(0)) {
                                case 'b':
                                    if (pair.second == "brush")
                                        m_terrain = RoomTerrainEnum::BRUSH;
                                    else // building
                                        m_terrain = RoomTerrainEnum::INDOORS;
                                    break;
                                case 'c':
                                    if (pair.second == "cavern")
                                        m_terrain = RoomTerrainEnum::CAVERN;
                                    else // city
                                        m_terrain = RoomTerrainEnum::CITY;
                                    break;
                                case 'f':
                                    if (pair.second == "field")
                                        m_terrain = RoomTerrainEnum::FIELD;
                                    else // forest
                                        m_terrain = RoomTerrainEnum::FOREST;
                                    break;
                                case 'h':
                                    // hills
                                    m_terrain = RoomTerrainEnum::HILLS;
                                    break;
                                case 'm':
                                    // mountains
                                    m_terrain = RoomTerrainEnum::MOUNTAINS;
                                    break;
                                case 'r':
                                    if (pair.second == "rapids")
                                        m_terrain = RoomTerrainEnum::RAPIDS;
                                    else // road
                                        m_terrain = RoomTerrainEnum::ROAD;
                                    break;
                                case 's':
                                    // shallows
                                    m_terrain = RoomTerrainEnum::SHALLOW;
                                    break;
                                case 't':
                                    // tunnel
                                    m_terrain = RoomTerrainEnum::TUNNEL;
                                    break;
                                case 'u':
                                    // underwater
                                    m_terrain = RoomTerrainEnum::UNDERWATER;
                                    break;
                                case 'w':
                                    // water
                                    m_terrain = RoomTerrainEnum::WATER;
                                    break;
                                default:
                                    qWarning() << "Unknown terrain type"
                                               << mmqt::toQByteArrayLatin1(pair.second);
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
                break;
            case 'm':
                if (line.startsWith("movement")) {
                    if (m_descriptionReady) {
                        // We are most likely in a fall room where the prompt is not shown
                        move();
                    }
                    if (attributes.empty()) {
                        // movement/
                        m_move = CommandEnum::NONE;
                        break;
                    }
                    for (const auto &pair : attributes) {
                        if (pair.first.empty() || pair.second.empty())
                            continue;
                        switch (pair.first.at(0)) {
                        case 'd':
                            if (pair.first == "dir") {
                                switch (pair.second.at(0)) {
                                case 'n':
                                    m_move = CommandEnum::NORTH;
                                    break;
                                case 's':
                                    m_move = CommandEnum::SOUTH;
                                    break;
                                case 'e':
                                    m_move = CommandEnum::EAST;
                                    break;
                                case 'w':
                                    m_move = CommandEnum::WEST;
                                    break;
                                case 'u':
                                    m_move = CommandEnum::UP;
                                    break;
                                case 'd':
                                    m_move = CommandEnum::DOWN;
                                    break;
                                default:
                                    qWarning() << "Unknown movement dir"
                                               << mmqt::toQByteArrayLatin1(pair.second);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            case 'w':
                if (line.startsWith("weather")) {
                    m_lineFlags.insert(LineFlagEnum::WEATHER);
                }
                break;
            case 's':
                if (line.startsWith("status")) {
                    m_lineFlags.insert(LineFlagEnum::STATUS);

                } else if (line.startsWith("snoop")) {
                    m_lineFlags.insert(LineFlagEnum::SNOOP);
                    if (length > 13) {
                        m_snoopChar = line.at(13);
                    }
                }
                break;
            }
        }
        break;

    case XmlModeEnum::ROOM:
        if (length > 0) {
            switch (line.at(0)) {
            case 'c':
                if (line.startsWith("character")) {
                    m_xmlMode = XmlModeEnum::CHARACTER;
                    m_lineFlags.insert(LineFlagEnum::CHARACTER);
                }
                break;
            case 'g':
                if (line.startsWith("gratuitous") && getConfig().parser.removeXmlTags) {
                    m_gratuitous = true;
                }
                break;
            case 'e':
                if (line.startsWith("exits")) {
                    m_exits = nullString; // Reset string since payload can be from the 'exit' command
                    m_xmlMode = XmlModeEnum::EXITS;
                    m_lineFlags.insert(LineFlagEnum::EXITS);
                    m_descriptionReady = true;
                }
                break;
            case 'n':
                if (line.startsWith("name")) {
                    m_xmlMode = XmlModeEnum::NAME;
                    m_lineFlags.insert(LineFlagEnum::NAME);
                }
                break;
            case 'd':
                if (line.startsWith("description")) {
                    m_xmlMode = XmlModeEnum::DESCRIPTION;
                    // might be empty but valid description
                    m_roomDesc = RoomDesc{""};
                    m_lineFlags.insert(LineFlagEnum::DESCRIPTION);
                }
                break;
            case 't': // terrain tag only comes up in blindness or fog
                if (line.startsWith("terrain")) {
                    m_xmlMode = XmlModeEnum::TERRAIN;
                    m_lineFlags.insert(LineFlagEnum::TERRAIN);
                }
                break;
            case 'h': // Gods have an "Obvious exits" header
                if (line.startsWith("header")) {
                    m_xmlMode = XmlModeEnum::HEADER;
                    m_lineFlags.insert(LineFlagEnum::HEADER);
                    m_descriptionReady = true;
                }
                break;
            case C_SLASH:
                if (line.startsWith("/room")) {
                    m_xmlMode = XmlModeEnum::NONE;
                    m_lineFlags.remove(LineFlagEnum::ROOM);
                    m_descriptionReady = true;
                } else if (line.startsWith("/gratuitous")) {
                    m_gratuitous = false;
                }
                break;
            }
        }
        break;
    case XmlModeEnum::NAME:
        if (line.startsWith("/name")) {
            m_xmlMode = XmlModeEnum::ROOM;
            m_lineFlags.remove(LineFlagEnum::NAME);
        }
        break;
    case XmlModeEnum::DESCRIPTION:
        if (length > 0) {
            switch (line.at(0)) {
            case C_SLASH:
                if (line.startsWith("/description")) {
                    m_xmlMode = XmlModeEnum::ROOM;
                    m_lineFlags.remove(LineFlagEnum::DESCRIPTION);
                }
                break;
            }
        }
        break;
    case XmlModeEnum::EXITS:
        if (length > 0) {
            switch (line.at(0)) {
            case C_SLASH:
                if (line.startsWith("/exits")) {
                    std::ostringstream os;
                    parseExits(os);
                    m_lineToUser.append(mmqt::toQByteArrayLatin1(snoopToUser(os.str())));
                    m_exitsReady = true;
                    m_lineFlags.remove(LineFlagEnum::EXITS);
                    if (m_lineFlags.contains(LineFlagEnum::ROOM))
                        m_xmlMode = XmlModeEnum::ROOM;
                    else
                        m_xmlMode = XmlModeEnum::NONE;
                }
                break;
            }
        }
        break;
    case XmlModeEnum::PROMPT:
        if (length > 0) {
            switch (line.at(0)) {
            case 'c':
                if (line.startsWith("character")) {
                    m_xmlMode = XmlModeEnum::CHARACTER;
                    m_lineFlags.insert(LineFlagEnum::CHARACTER);
                }
                break;
            case C_SLASH:
                if (line.startsWith("/prompt")) {
                    m_xmlMode = XmlModeEnum::NONE;
                    m_lineFlags.remove(LineFlagEnum::PROMPT);
                    m_overrideSendPrompt = false;

                    const QString copy = normalizeStringCopy(m_lastPrompt);
                    sendPromptLineEvent(copy.toLatin1());
                    parsePrompt(copy);

                    const auto &config = getConfig();
                    if (!config.parser.removeXmlTags) {
                        m_lastPrompt.replace(ampersand, ampersandTemplate);
                        m_lastPrompt.replace(greaterThanChar, greaterThanTemplate);
                        m_lastPrompt.replace(lessThanChar, lessThanTemplate);
                        m_lastPrompt = "<prompt>" + m_lastPrompt + "</prompt>";
                    }

                    if (m_descriptionReady) {
                        if (!m_exitsReady && config.mumeNative.emulatedExits) {
                            m_exitsReady = true;
                            std::ostringstream os;
                            emulateExits(os, m_move);
                            sendToUser(mmqt::toQByteArrayLatin1(snoopToUser(os.str())));
                        }
                        move();
                    }
                }
                break;
            }
        }
        break;
    case XmlModeEnum::TERRAIN:
        if (length > 0) {
            switch (line.at(0)) {
            case C_SLASH:
                if (line.startsWith("/terrain")) {
                    m_xmlMode = XmlModeEnum::ROOM;
                    m_lineFlags.remove(LineFlagEnum::TERRAIN);
                }
                break;
            }
        }
        break;
    case XmlModeEnum::HEADER:
        if (length > 0) {
            switch (line.at(0)) {
            case C_SLASH:
                if (line.startsWith("/header")) {
                    m_xmlMode = XmlModeEnum::ROOM;
                    m_lineFlags.remove(LineFlagEnum::HEADER);
                }
                break;
            }
        }
        break;
    case XmlModeEnum::CHARACTER:
        if (length > 0) {
            switch (line.at(0)) {
            case C_SLASH:
                if (line.startsWith("/character")) {
                    if (m_lineFlags.isPrompt())
                        m_xmlMode = XmlModeEnum::PROMPT;
                    else
                        m_xmlMode = XmlModeEnum::ROOM;
                    m_lineFlags.remove(LineFlagEnum::CHARACTER);
                }
                break;
            }
        }
        break;
    }

    if (!getConfig().parser.removeXmlTags) {
        m_lineToUser.append(lessThanChar).append(line).append(greaterThanChar);
    }

    return true;
}

void MumeXmlParser::stripXmlEntities(QByteArray &ch)
{
    ch.replace(greaterThanTemplate, greaterThanChar);
    ch.replace(lessThanTemplate, lessThanChar);
    ch.replace(ampersandTemplate, ampersand);
}

QByteArray MumeXmlParser::characters(QByteArray &ch)
{
    if (ch.isEmpty()) {
        return ch;
    }

    // replace > and < chars
    stripXmlEntities(ch);

    const auto &config = getConfig();
    m_stringBuffer = QString::fromLatin1(ch);

    if (m_lineFlags.isSnoop() && m_stringBuffer.length() > 3 && m_stringBuffer.at(0) == C_AMPERSAND
        && m_stringBuffer.at(2) == C_SPACE) {
        // Remove snoop prefix (i.e. "&J Exits: north.")
        m_stringBuffer = m_stringBuffer.mid(3);
    }

    QByteArray toUser;

    const XmlModeEnum mode = [this]() -> XmlModeEnum {
        if (m_lineFlags.isPrompt())
            return XmlModeEnum::PROMPT;
        else if (m_lineFlags.isExits())
            return XmlModeEnum::EXITS;
        else if (m_lineFlags.isName())
            return XmlModeEnum::NAME;
        else if (m_lineFlags.isDescription())
            return XmlModeEnum::DESCRIPTION;
        else if (m_lineFlags.isRoom())
            return XmlModeEnum::ROOM;
        return m_xmlMode;
    }();

    switch (mode) {
    case XmlModeEnum::NONE: // non room info
        m_stringBuffer = normalizeStringCopy(m_stringBuffer);
        if (m_stringBuffer.isEmpty()) { // standard end of description parsed
            if (m_descriptionReady && !m_exitsReady && config.mumeNative.emulatedExits) {
                m_exitsReady = true;
                std::ostringstream os;
                emulateExits(os, m_move);
                sendToUser(mmqt::toQByteArrayLatin1(snoopToUser(os.str())));
            }
        } else {
            m_lineFlags.insert(LineFlagEnum::NONE);
        }
        toUser.append(ch);
        break;

    case XmlModeEnum::ROOM: // dynamic line
        if (!m_descriptionReady && m_roomDesc.has_value()) {
            m_roomContents = RoomContents{m_roomContents.value_or(RoomContents{}).toQString()
                                          + normalizeStringCopy(m_stringBuffer)};
        }
        toUser.append(ch);
        break;

    case XmlModeEnum::NAME:
        m_roomName = RoomName{normalizeStringCopy(m_stringBuffer)};
        toUser.append(ch);
        break;

    case XmlModeEnum::DESCRIPTION: // static line
        if (!m_descriptionReady) {
            m_roomDesc = RoomDesc{m_roomDesc.value_or(RoomDesc{}).toQString()
                                  + normalizeStringCopy(m_stringBuffer)};
        }
        if (!m_gratuitous) {
            toUser.append(ch);
        }
        break;

    case XmlModeEnum::EXITS:
        m_exits += m_stringBuffer;
        break;

    case XmlModeEnum::PROMPT:
        // Store prompts in case an internal command is executed
        m_lastPrompt += m_stringBuffer.toLatin1();
        toUser.append(ch);
        break;

    case XmlModeEnum::CHARACTER:
    case XmlModeEnum::HEADER:
    case XmlModeEnum::TERRAIN:
    default:
        toUser.append(ch);
        break;
    }

    if (!getConfig().parser.removeXmlTags) {
        toUser.replace(ampersand, ampersandTemplate);
        toUser.replace(greaterThanChar, greaterThanTemplate);
        toUser.replace(lessThanChar, lessThanTemplate);
    }
    return toUser;
}

void MumeXmlParser::move()
{
    m_descriptionReady = false;

    if (!m_roomName.has_value() || // blindness
        Patterns::matchNoDescriptionPatterns(
            m_roomName.value()
                .toQString())) { // non standard end of description parsed (fog, dark or so ...)
        m_roomName.reset();
        m_roomDesc.reset();
        m_roomContents.reset();
    }

    const auto emitEvent = [this]() {
        auto ev = ParseEvent::createEvent(m_move,
                                          m_roomName.value_or(RoomName{}),
                                          m_roomDesc.value_or(RoomDesc{}),
                                          m_roomContents.value_or(RoomContents{}),
                                          m_terrain,
                                          m_exitsFlags,
                                          m_promptFlags,
                                          m_connectedRoomFlags);
        emit sig_handleParseEvent(SigParseEvent{ev});
    };

    if (m_queue.isEmpty()) {
        emitEvent();
    } else {
        const CommandEnum c = m_queue.dequeue();
        // Ignore scouting unless it forced movement via a one-way
        if (c != CommandEnum::SCOUT || m_move != CommandEnum::LOOK) {
            pathChanged();
            emitEvent();
            if (c != m_move) {
                m_queue.clear();
            }
        }
    }
    m_move = CommandEnum::LOOK;
}

void MumeXmlParser::parseMudCommands(const QString &str)
{
    // REVISIT: Add XML tag-based actions that match on a given LineFlag
    const auto stdString = mmqt::toStdStringLatin1(str);
    if (evalActionMap(StringView{stdString}))
        return;
}

std::string MumeXmlParser::snoopToUser(const std::string_view str)
{
    std::ostringstream os;
    bool snoopPrefix = m_snoopChar.has_value();
    for (const char c : str) {
        if (snoopPrefix) {
            os << C_AMPERSAND << m_snoopChar.value() << C_SPACE;
            snoopPrefix = false;
        }
        os << c;
        if (c == C_NEWLINE && m_snoopChar.has_value())
            snoopPrefix = true;
    }
    return os.str();
}
