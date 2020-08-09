// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "mumexmlparser.h"

#include <sstream>
#include <QByteArray>
#include <QString>

#include "../clock/mumeclock.h"
#include "../configuration/configuration.h"
#include "../expandoracommon/parseevent.h"
#include "../global/TextUtils.h"
#include "../pandoragroup/mmapper2group.h"
#include "../proxy/telnetfilter.h"
#include "ExitsFlags.h"
#include "PromptFlags.h"
#include "abstractparser.h"
#include "parserutils.h"
#include "patterns.h"

class MapData;

namespace {
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

MumeXmlParser::MumeXmlParser(
    MapData *md, MumeClock *mc, ProxyParserApi proxy, GroupManagerApi group, QObject *parent)
    : AbstractParser(md, mc, proxy, group, parent)
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

void MumeXmlParser::parseNewMudInput(const TelnetData &data)
{
    switch (data.type) {
    case TelnetDataEnum::DELAY: // Twiddlers
    case TelnetDataEnum::MENU_PROMPT:
    case TelnetDataEnum::LOGIN:
    case TelnetDataEnum::LOGIN_PASSWORD:
        m_lastPrompt = data.line;
        stripXmlEntities(m_lastPrompt);
        parse(data);
        break;
    case TelnetDataEnum::UNKNOWN:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "OTHER";
            (*debugStream) << "***ETYPE***";
        }
        // Login prompt and IAC-GA
        parse(data);
        break;

    case TelnetDataEnum::PROMPT:
    case TelnetDataEnum::LF:
    case TelnetDataEnum::CRLF:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "CRLF";
            (*debugStream) << "***ETYPE***";
        }
        // XML and prompts
        parse(data);
        break;
    }
    if (XPS_DEBUG_TO_FILE) {
        (*debugStream) << "***S***";
        (*debugStream) << data.line;
        (*debugStream) << "***E***";
    }
}

void MumeXmlParser::parse(const TelnetData &data)
{
    const QByteArray &line = data.line;
    m_lineToUser.clear();
    m_lineFlags.remove(LineFlagEnum::NONE);
    if (!m_lineFlags.isSnoop())
        m_snoopChar.reset();

    for (const char c : line) {
        if (m_readingTag) {
            if (c == '>') {
                // send tag
                if (!m_tempTag.isEmpty()) {
                    element(m_tempTag);
                }

                m_tempTag.clear();

                m_readingTag = false;
                continue;
            }
            m_tempTag.append(c);

        } else {
            if (c == '<') {
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
        const auto isGoAhead = [](const TelnetDataEnum type) -> bool {
            switch (type) {
            case TelnetDataEnum::DELAY:
            case TelnetDataEnum::LOGIN:
            case TelnetDataEnum::LOGIN_PASSWORD:
            case TelnetDataEnum::MENU_PROMPT:
            case TelnetDataEnum::PROMPT:
                return true;
            case TelnetDataEnum::CRLF:
            case TelnetDataEnum::LF:
            case TelnetDataEnum::UNKNOWN:
                return false;
            }
            return false;
        };
        sendToUser(m_lineToUser, isGoAhead(data.type));

        {
            // Simplify the output and run actions
            QByteArray temp = m_lineToUser;
            if (!getConfig().parser.removeXmlTags) {
                stripXmlEntities(temp);
            }
            QString tempStr = temp;
            tempStr = normalizeStringCopy(tempStr.trimmed());
            if (m_snoopChar.has_value() && tempStr.length() > 3 && tempStr.at(0) == '&'
                && tempStr.at(1) == m_snoopChar.value() && tempStr.at(2) == ' ') {
                // Remove snoop prefix (i.e. "&J Exits: north.")
                tempStr = tempStr.mid(3);
            }
            parseMudCommands(tempStr);
        }
    }
}

bool MumeXmlParser::element(const QByteArray &line)
{
    const int length = line.length();

    switch (m_xmlMode) {
    case XmlModeEnum::NONE:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/snoop")) {
                    m_lineFlags.remove(LineFlagEnum::SNOOP);
                    if (m_descriptionReady) {
                        if (!m_exitsReady && getConfig().mumeNative.emulatedExits) {
                            m_exitsReady = true;
                            std::ostringstream os;
                            emulateExits(os, m_move);
                            sendToUser(::toQByteArrayLatin1(snoopToUser(os.str())));
                        }
                        m_promptFlags.reset(); // Don't trust god prompts
                        if (!m_queue.isEmpty() && m_move != CommandEnum::LOOK) // Remove follows
                            m_queue.dequeue();
                        if (m_move != CommandEnum::LOOK)
                            m_queue.enqueue(m_move);
                        move();
                    }

                } else if (line.startsWith("/status")) {
                    m_lineFlags.remove(LineFlagEnum::STATUS);

                } else if (line.startsWith("/weather")) {
                    m_lineFlags.remove(LineFlagEnum::WEATHER);
                    // Certain weather events happen on ticks
                    m_mumeClock->parseWeather(m_stringBuffer);

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
                    m_dynamicRoomDesc.reset();
                    m_staticRoomDesc.reset();
                    m_exits = nullString;
                    m_promptFlags.reset();
                    m_exitsFlags.reset();
                    m_connectedRoomFlags.reset();
                    m_lineFlags.insert(LineFlagEnum::ROOM);
                }
                break;
            case 'm':
                if (m_descriptionReady) {
                    // We are most likely in a fall room where the prompt is not shown
                    move();
                }
                if (length > 8) {
                    switch (line.at(8)) {
                    case ' ':
                        if (length > 13) {
                            switch (line.at(13)) {
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
                            }
                        }
                        break;
                    case '/':
                        m_move = CommandEnum::NONE;
                        break;
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
            case 'h':
                if (line.startsWith("header")) {
                    m_xmlMode = XmlModeEnum::HEADER;
                    m_lineFlags.insert(LineFlagEnum::HEADER);
                    if (m_descriptionReady) {
                        // Prevent emulated exits from firing due to god exits
                        m_exitsReady = true;
                    }
                }
                break;
            }
        }
        break;

    case XmlModeEnum::ROOM:
        if (length > 0) {
            switch (line.at(0)) {
            case 'g':
                if (line.startsWith("gratuitous") && getConfig().parser.removeXmlTags) {
                    m_gratuitous = true;
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
                    m_staticRoomDesc = RoomStaticDesc{""};
                    m_lineFlags.insert(LineFlagEnum::DESCRIPTION);
                }
                break;
            case 't': // terrain tag only comes up in blindness or fog
                if (line.startsWith("terrain")) {
                    m_xmlMode = XmlModeEnum::TERRAIN;
                    m_lineFlags.insert(LineFlagEnum::TERRAIN);
                }
                break;

            case '/':
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
            case '/':
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
            case '/':
                if (line.startsWith("/exits")) {
                    std::ostringstream os;
                    parseExits(os);
                    m_lineToUser.append(::toQByteArrayLatin1(snoopToUser(os.str())));
                    m_exitsReady = true;
                    m_lineFlags.remove(LineFlagEnum::EXITS);
                    m_xmlMode = XmlModeEnum::NONE;
                }
                break;
            }
        }
        break;
    case XmlModeEnum::PROMPT:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/prompt")) {
                    m_xmlMode = XmlModeEnum::NONE;
                    m_lineFlags.remove(LineFlagEnum::PROMPT);
                    m_overrideSendPrompt = false;
                }
                break;
            }
        }
        break;
    case XmlModeEnum::TERRAIN:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
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
            case '/':
                if (line.startsWith("/header")) {
                    m_xmlMode = XmlModeEnum::NONE;
                    m_lineFlags.remove(LineFlagEnum::HEADER);
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
    QByteArray toUser;

    if (ch.isEmpty()) {
        return toUser;
    }

    // replace > and < chars
    stripXmlEntities(ch);

    const auto &config = getConfig();
    m_stringBuffer = QString::fromLatin1(ch);

    if (m_lineFlags.isSnoop() && m_stringBuffer.length() > 3 && m_stringBuffer.at(0) == '&'
        && m_stringBuffer.at(2) == ' ') {
        // Remove snoop prefix (i.e. "&J Exits: north.")
        m_stringBuffer = m_stringBuffer.mid(3);
    }

    switch (m_xmlMode) {
    case XmlModeEnum::NONE: // non room info
        m_stringBuffer = normalizeStringCopy(m_stringBuffer.trimmed());
        if (m_stringBuffer.isEmpty()) { // standard end of description parsed
            if (m_descriptionReady && !m_exitsReady && config.mumeNative.emulatedExits) {
                m_exitsReady = true;
                std::ostringstream os;
                emulateExits(os, m_move);
                sendToUser(::toQByteArrayLatin1(snoopToUser(os.str())));
            }
        } else {
            m_lineFlags.insert(LineFlagEnum::NONE);
        }
        toUser.append(ch);
        break;

    case XmlModeEnum::ROOM: // dynamic line
        m_dynamicRoomDesc = RoomDynamicDesc{
            m_dynamicRoomDesc.value_or(RoomDynamicDesc{}).toQString()
            + normalizeStringCopy(m_stringBuffer.simplified().append("\n"))};
        toUser.append(ch);
        break;

    case XmlModeEnum::NAME:
        m_roomName = RoomName{normalizeStringCopy(m_stringBuffer)};
        toUser.append(ch);
        break;

    case XmlModeEnum::DESCRIPTION: // static line
        m_staticRoomDesc = RoomStaticDesc{
            m_staticRoomDesc.value_or(RoomStaticDesc{}).toQString()
            + normalizeStringCopy(m_stringBuffer.simplified().append("\n"))};
        if (!m_gratuitous) {
            toUser.append(ch);
        }
        break;

    case XmlModeEnum::EXITS:
        m_exits += m_stringBuffer;
        break;

    case XmlModeEnum::PROMPT:
        // Store prompts in case an internal command is executed
        m_lastPrompt = m_stringBuffer.toLatin1();
        if (!getConfig().parser.removeXmlTags) {
            m_lastPrompt.replace(ampersand, ampersandTemplate);
            m_lastPrompt.replace(greaterThanChar, greaterThanTemplate);
            m_lastPrompt.replace(lessThanChar, lessThanTemplate);
            m_lastPrompt = "<prompt>" + m_lastPrompt + "</prompt>";
        }
        sendPromptLineEvent(normalizeStringCopy(m_stringBuffer).toLatin1());
        if (m_descriptionReady) {
            if (!m_exitsReady && config.mumeNative.emulatedExits) {
                m_exitsReady = true;
                std::ostringstream os;
                emulateExits(os, m_move);
                sendToUser(::toQByteArrayLatin1(snoopToUser(os.str())));
            }
            parsePrompt(normalizeStringCopy(m_stringBuffer));
            move();
        }
        toUser.append(ch);
        break;

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
        m_dynamicRoomDesc.reset();
        m_staticRoomDesc.reset();
    }

    const auto emitEvent = [this]() {
        auto ev = ParseEvent::createEvent(m_move,
                                          m_roomName.value_or(RoomName{}),
                                          m_dynamicRoomDesc.value_or(RoomDynamicDesc{}),
                                          m_staticRoomDesc.value_or(RoomStaticDesc{}),
                                          m_exitsFlags,
                                          m_promptFlags,
                                          m_connectedRoomFlags);
        emit event(SigParseEvent{ev});
    };

    if (m_queue.isEmpty()) {
        emitEvent();
    } else {
        const CommandEnum c = m_queue.dequeue();
        // Ignore scouting unless it forced movement via a one-way
        if (c != CommandEnum::SCOUT || (c == CommandEnum::SCOUT && m_move != CommandEnum::LOOK)) {
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
    const auto stdString = ::toStdStringLatin1(str);
    if (evalActionMap(StringView{stdString}))
        return;
}

std::string MumeXmlParser::snoopToUser(const std::string_view &str)
{
    std::ostringstream os;
    bool snoopPrefix = m_snoopChar.has_value();
    for (const auto c : str) {
        if (snoopPrefix) {
            os << '&' << m_snoopChar.value() << ' ';
            snoopPrefix = false;
        }
        os << c;
        if (c == '\n' && m_snoopChar.has_value())
            snoopPrefix = true;
    }
    return os.str();
}
