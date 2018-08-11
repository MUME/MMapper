/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/
#include "mumexmlparser.h"

#include <QByteArray>
#include <QString>

#include "../clock/mumeclock.h"
#include "../configuration/configuration.h"
#include "../expandoracommon/parseevent.h"
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

const QByteArray MumeXmlParser::greaterThanChar(">");
const QByteArray MumeXmlParser::lessThanChar("<");
const QByteArray MumeXmlParser::greaterThanTemplate("&gt;");
const QByteArray MumeXmlParser::lessThanTemplate("&lt;");
const QByteArray MumeXmlParser::ampersand("&");
const QByteArray MumeXmlParser::ampersandTemplate("&amp;");

MumeXmlParser::MumeXmlParser(MapData *md, MumeClock *mc, QObject *parent)
    : AbstractParser(md, mc, parent)
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

void MumeXmlParser::parseNewMudInput(IncomingData &data)
{
    switch (data.type) {
    case TelnetDataType::DELAY:
    case TelnetDataType::MENU_PROMPT:
    case TelnetDataType::LOGIN:
    case TelnetDataType::LOGIN_PASSWORD:
    case TelnetDataType::TELNET:
    case TelnetDataType::SPLIT:
    case TelnetDataType::UNKNOWN:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "OTHER";
            (*debugStream) << "***ETYPE***";
        }
        // Login prompt and IAC-GA
        parse(data.line);
        break;

    case TelnetDataType::PROMPT:
    case TelnetDataType::LF:
    case TelnetDataType::LFCR:
    case TelnetDataType::CRLF:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "CRLF";
            (*debugStream) << "***ETYPE***";
        }
        // XML and prompts
        parse(data.line);
        break;
    }
    if (XPS_DEBUG_TO_FILE) {
        (*debugStream) << "***S***";
        (*debugStream) << data.line;
        (*debugStream) << "***E***";
    }
}

void MumeXmlParser::parse(const QByteArray &line)
{
    m_lineToUser.clear();
    int index;

    for (index = 0; index < line.size(); index++) {
        if (m_readingTag) {
            if (line.at(index) == '>') {
                //send tag
                if (!m_tempTag.isEmpty()) {
                    element(m_tempTag);
                }

                m_tempTag.clear();

                m_readingTag = false;
                continue;
            }
            m_tempTag.append(line.at(index));

        } else {
            if (line.at(index) == '<') {
                m_lineToUser.append(characters(m_tempCharacters));
                m_tempCharacters.clear();

                m_readingTag = true;
                continue;
            }
            m_tempCharacters.append(line.at(index));
        }
    }

    if (!m_readingTag) {
        m_lineToUser.append(characters(m_tempCharacters));
        m_tempCharacters.clear();
    }

    if (!m_lineToUser.isEmpty()) {
        emit sig_sendToUser(m_lineToUser);

        if (m_readStatusTag) {
            m_readStatusTag = false;
            if (Config().groupManager.state != GroupManagerState::Off) {
                QString temp(m_lineToUser.simplified());
                ParserUtils::removeAnsiMarks(temp);
                if (Patterns::matchScore(temp)) {
                    // inform groupManager
                    emit sendScoreLineEvent(temp.toLatin1());
                }
            }
        }
    }
}

bool MumeXmlParser::element(const QByteArray &line)
{
    int length = line.length();

    switch (m_xmlMode) {
    case XmlMode::NONE:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/xml")) {
                    emit sig_sendToUser("[MMapper] Mapper cannot function without XML mode\n");
                    emptyQueue();
                }
                break;
            case 'p':
                if (line.startsWith("prompt")) {
                    m_xmlMode = XmlMode::PROMPT;
                }
                break;
            case 'e':
                if (line.startsWith("exits")) {
                    m_exits = nullString; // Reset string since payload can be from the 'exit' command
                    m_xmlMode = XmlMode::EXITS;
                }
                break;
            case 'r':
                if (line.startsWith("room")) {
                    m_xmlMode = XmlMode::ROOM;
                }
                break;
            case 'm':
                if (length > 8) {
                    switch (line.at(8)) {
                    case ' ':
                        if (length > 13) {
                            switch (line.at(13)) {
                            case 'n':
                                m_move = CommandIdType::NORTH;
                                break;
                            case 's':
                                m_move = CommandIdType::SOUTH;
                                break;
                            case 'e':
                                m_move = CommandIdType::EAST;
                                break;
                            case 'w':
                                m_move = CommandIdType::WEST;
                                break;
                            case 'u':
                                m_move = CommandIdType::UP;
                                break;
                            case 'd':
                                m_move = CommandIdType::DOWN;
                                break;
                            };
                        }
                        break;
                    case '/':
                        m_move = CommandIdType::NONE;
                        break;
                    }
                };
                break;
            case 'w':
                if (line.startsWith("weather")) {
                    m_readWeatherTag = true;
                }
                break;
            case 's':
                if (line.startsWith("status")) {
                    m_readStatusTag = true;
                    m_xmlMode = XmlMode::NONE;
                } else if (line.startsWith("snoop")) {
                    m_readSnoopTag = true;
                }
                break;
            }
        };
        break;

    case XmlMode::ROOM:
        if (length > 0) {
            switch (line.at(0)) {
            case 'g':
                if (line.startsWith("gratuitous")) {
                    m_gratuitous = true;
                }
                break;
            case 'n':
                if (line.startsWith("name")) {
                    m_xmlMode = XmlMode::NAME;
                    m_roomName = emptyString; // might be empty but valid room name
                }
                break;
            case 'd':
                if (line.startsWith("description")) {
                    m_xmlMode = XmlMode::DESCRIPTION;
                    m_staticRoomDesc = emptyString; // might be empty but valid description
                }
                break;
            case 't': // terrain tag only comes up in blindness or fog
                if (line.startsWith("terrain")) {
                    m_xmlMode = XmlMode::TERRAIN;
                }
                break;

            case '/':
                if (line.startsWith("/room")) {
                    m_xmlMode = XmlMode::NONE;
                } else if (line.startsWith("/gratuitous")) {
                    m_gratuitous = false;
                }
                break;
            }
        }
        break;
    case XmlMode::NAME:
        if (line.startsWith("/name")) {
            m_xmlMode = XmlMode::ROOM;
        }
        break;
    case XmlMode::DESCRIPTION:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/description")) {
                    m_xmlMode = XmlMode::ROOM;
                }
                break;
            }
        }
        break;
    case XmlMode::EXITS:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/exits")) {
                    parseExits();
                    m_xmlMode = XmlMode::NONE;
                }
                break;
            }
        }
        break;
    case XmlMode::PROMPT:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/prompt")) {
                    m_xmlMode = XmlMode::NONE;
                }
                break;
            }
        }
        break;
    case XmlMode::TERRAIN:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/terrain")) {
                    m_xmlMode = XmlMode::ROOM;
                    m_readingRoomDesc = true;
                }
                break;
            }
        }
        break;
    }

    if (!Config().parser.removeXmlTags) {
        // REVISIT: avoid temporary here; just append 3 times.
        m_lineToUser.append("<" + line + ">");
    }
    return true;
}

QByteArray MumeXmlParser::characters(QByteArray &ch)
{
    QByteArray toUser;

    if (ch.isEmpty()) {
        return toUser;
    }

    // replace > and < chars
    ch.replace(greaterThanTemplate, greaterThanChar);
    ch.replace(lessThanTemplate, lessThanChar);
    ch.replace(ampersandTemplate, ampersand);

    // Store prompts in case an internal command is executed
    if (m_xmlMode == XmlMode::PROMPT) {
        m_lastPrompt = ch;
    }

    const auto &config = Config();
    m_stringBuffer = config.parser.utf8Charset ? QString::fromUtf8(ch) : QString::fromLatin1(ch);

    if (m_readSnoopTag && m_stringBuffer.length() > 3 && m_stringBuffer.at(0) == '&'
        && m_stringBuffer.at(2) == ' ') {
        // Remove snoop prefix (i.e. "&J Exits: north.")
        m_stringBuffer = m_stringBuffer.mid(3);
    }

    switch (m_xmlMode) {
    case XmlMode::NONE:                 //non room info
        if (m_stringBuffer.trimmed().isEmpty()) { // standard end of description parsed
            if (m_readingRoomDesc) {
                m_readingRoomDesc = false; // we finished read desc mode
                m_descriptionReady = true;
                if (config.mumeNative.emulatedExits) {
                    emulateExits();
                }
            }
        } else {
            parseMudCommands(m_stringBuffer);
        }
        if (m_readSnoopTag) {
            if (m_descriptionReady) {
                m_promptFlags.reset(); // Don't trust god prompts
                queue.enqueue(m_move);
                emit showPath(queue, true);
                move();
                m_readSnoopTag = false;
            }
        }
        toUser.append(ch);
        break;

    case XmlMode::ROOM: // dynamic line
        m_dynamicRoomDesc += m_stringBuffer.simplified() + "\n";
        toUser.append(ch);
        break;

    case XmlMode::NAME:
        if (m_descriptionReady) {
            move();
        }

        m_readingRoomDesc = true; //start of read desc mode
        m_descriptionReady = false;
        m_roomName = m_stringBuffer;
        m_dynamicRoomDesc = nullString;
        m_staticRoomDesc = nullString;
        m_exits = nullString;
        m_exitsFlags.reset();

        toUser.append(ch);
        break;

    case XmlMode::DESCRIPTION: // static line
        m_staticRoomDesc += m_stringBuffer.simplified() + "\n";
        if (!m_gratuitous) {
            toUser.append(ch);
        }
        break;

    case XmlMode::EXITS:
        m_exits += m_stringBuffer;
        if (m_readingRoomDesc) {
            m_readingRoomDesc = false;
            m_descriptionReady = true;
        }
        break;

    case XmlMode::PROMPT:
        emit sendPromptLineEvent(m_stringBuffer.toLatin1());
        if (m_readingRoomDesc) {       // fixes compact mode
            m_readingRoomDesc = false; // we finished read desc mode
            m_descriptionReady = true;
            if (config.mumeNative.emulatedExits) {
                emulateExits();
            }
        }
        if (m_descriptionReady) {
            parsePrompt(m_stringBuffer);
            move();
        } else {
            if (!queue.isEmpty()) {
                queue.dequeue();
                emit showPath(queue, true);
            }
        }

        toUser.append(ch);
        break;

    case XmlMode::TERRAIN:
    default:
        toUser.append(ch);
        break;
    }

    return toUser;
}

void MumeXmlParser::move()
{
    m_descriptionReady = false;

    if (m_roomName == emptyString || // blindness
        Patterns::matchNoDescriptionPatterns(
            m_roomName)) { // non standard end of description parsed (fog, dark or so ...)
        m_roomName = nullString;
        m_dynamicRoomDesc = nullString;
        m_staticRoomDesc = nullString;
    }

    const auto emitEvent = [this]() {
        auto ev = ParseEvent::createEvent(m_move,
                                          normalizeString(m_roomName),
                                          normalizeString(m_dynamicRoomDesc),
                                          normalizeString(m_staticRoomDesc),
                                          m_exitsFlags,
                                          m_promptFlags,
                                          m_connectedRoomFlags);
        emit event(SigParseEvent{ev});
        m_move = CommandIdType::LOOK;
    };

    if (queue.isEmpty()) {
        emitEvent();
    } else {
        const CommandIdType c = queue.dequeue();
        if (c != CommandIdType::SCOUT) {
            emit showPath(queue, false);
            emitEvent();
            if (c != m_move) {
                queue.clear();
            }
        }
    }
}

void MumeXmlParser::parseMudCommands(const QString &str)
{
    if (str.at(0) == ('Y')) {
        if (str.startsWith("You are dead!")) {
            queue.clear();
            emit showPath(queue, true);
            emit releaseAllPaths();
            markCurrentCommand();
            return;
        } else if (str.startsWith("You failed to climb")) {
            if (!queue.isEmpty()) {
                queue.dequeue();
            }
            queue.prepend(CommandIdType::NONE);
            emit showPath(queue, true);
            return;
        } else if (str.startsWith("You flee head")) {
            queue.enqueue(m_move);
            return;
        } else if (str.startsWith("You follow")) {
            queue.enqueue(m_move);
            return;
        } else if (str.startsWith("You quietly scout")) {
            queue.prepend(CommandIdType::SCOUT);
            return;
        }
    } else if (str.at(0) == 'T') {
        if (str.startsWith("The current time is")) {
            m_mumeClock->parseClockTime(str);
        }
    }
    if (str.endsWith("of the Third Age.")) {
        m_mumeClock->parseMumeTime(str);
    }

    // Certain weather events happen on ticks
    if (m_readWeatherTag) {
        m_readWeatherTag = false;
        m_mumeClock->parseWeather(str);
    }

    // parse regexps which force new char move
    if (Patterns::matchMoveForcePatterns(str)) {
        queue.enqueue(CommandIdType::NONE);
        emit showPath(queue, true);
        return;
    }
}
