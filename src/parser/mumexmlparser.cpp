// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

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

void MumeXmlParser::parseNewMudInput(const IncomingData &data)
{
    switch (data.type) {
    case TelnetDataType::DELAY: // Twiddlers
    case TelnetDataType::MENU_PROMPT:
    case TelnetDataType::LOGIN:
    case TelnetDataType::LOGIN_PASSWORD:
        m_lastPrompt = data.line;
        stripXmlEntities(m_lastPrompt);
        parse(data);
        break;
    case TelnetDataType::SPLIT:
    case TelnetDataType::UNKNOWN:
        if (XPS_DEBUG_TO_FILE) {
            (*debugStream) << "***STYPE***";
            (*debugStream) << "OTHER";
            (*debugStream) << "***ETYPE***";
        }
        // Login prompt and IAC-GA
        parse(data);
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
        parse(data);
        break;
    }
    if (XPS_DEBUG_TO_FILE) {
        (*debugStream) << "***S***";
        (*debugStream) << data.line;
        (*debugStream) << "***E***";
    }
}

void MumeXmlParser::parse(const IncomingData &data)
{
    const QByteArray &line = data.line;
    m_lineToUser.clear();
    int index;

    for (index = 0; index < line.size(); index++) {
        if (m_readingTag) {
            if (line.at(index) == '>') {
                // send tag
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
        const auto isGoAhead = [](TelnetDataType type) -> bool {
            switch (type) {
            case TelnetDataType::DELAY:
            case TelnetDataType::LOGIN:
            case TelnetDataType::LOGIN_PASSWORD:
            case TelnetDataType::MENU_PROMPT:
            case TelnetDataType::PROMPT:
                return true;
            case TelnetDataType::CRLF:
            case TelnetDataType::LFCR:
            case TelnetDataType::LF:
            case TelnetDataType::SPLIT:
            case TelnetDataType::UNKNOWN:
                return false;
            }
            return false;
        };
        sendToUser(m_lineToUser, isGoAhead(data.type));

        if (m_readStatusTag) {
            m_readStatusTag = false;
            QByteArray temp = m_lineToUser;
            if (!getConfig().parser.removeXmlTags) {
                stripXmlEntities(temp);
            }
            QString tempStr = temp;
            tempStr = normalizeStringCopy(tempStr.trimmed());
            if (Patterns::matchScore(tempStr)) {
                // inform groupManager
                temp = tempStr.toLocal8Bit();
                emit sendScoreLineEvent(temp);
            }
        }
    }
}

bool MumeXmlParser::element(const QByteArray &line)
{
    const int length = line.length();
    const XmlMode lastMode = m_xmlMode;

    switch (m_xmlMode) {
    case XmlMode::NONE:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/xml")) {
                    sendToUser("[MMapper] Mapper cannot function without XML mode\n");
                    queue.clear();
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
                    m_roomName = emptyString; // 'name' tag will not show up when blinded
                    m_descriptionReady = false;
                    m_exitsReady = false;
                    m_dynamicRoomDesc = nullString;
                    m_staticRoomDesc = nullString;
                    m_exits = nullString;
                    m_promptFlags.reset();
                    m_exitsFlags.reset();
                    m_connectedRoomFlags.reset();
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
            case 'h':
                if (line.startsWith("header")) {
                    m_xmlMode = XmlMode::HEADER;
                }
                break;
            }
        };
        break;

    case XmlMode::ROOM:
        if (length > 0) {
            switch (line.at(0)) {
            case 'g':
                if (line.startsWith("gratuitous") && getConfig().parser.removeXmlTags) {
                    m_gratuitous = true;
                }
                break;
            case 'n':
                if (line.startsWith("name")) {
                    m_xmlMode = XmlMode::NAME;
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
                    m_overrideSendPrompt = false;
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
                }
                break;
            }
        }
        break;
    case XmlMode::HEADER:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/header")) {
                    m_xmlMode = XmlMode::NONE;
                }
                break;
            }
        }
        break;
    }

    if (!getConfig().parser.removeXmlTags) {
        m_lineToUser.append(lessThanChar).append(line).append(greaterThanChar);
    }

    if (lastMode == XmlMode::PROMPT) {
        // Store prompts in case an internal command is executed
        m_lastPrompt = m_lineToUser;
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

    if (m_readSnoopTag && m_stringBuffer.length() > 3 && m_stringBuffer.at(0) == '&'
        && m_stringBuffer.at(2) == ' ') {
        // Remove snoop prefix (i.e. "&J Exits: north.")
        m_stringBuffer = m_stringBuffer.mid(3);
    }

    switch (m_xmlMode) {
    case XmlMode::NONE: // non room info
        m_stringBuffer = normalizeStringCopy(m_stringBuffer.trimmed());
        if (m_stringBuffer.isEmpty()) { // standard end of description parsed
            if (!m_exitsReady) {
                m_exitsReady = true; // we finished read desc mode
                m_descriptionReady = true;
                if (config.mumeNative.emulatedExits) {
                    emulateExits(m_move);
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
        m_dynamicRoomDesc += normalizeStringCopy(m_stringBuffer.simplified().append("\n"));
        toUser.append(ch);
        break;

    case XmlMode::NAME:
        if (m_descriptionReady) {
            move();
        }
        m_roomName = normalizeStringCopy(m_stringBuffer);
        toUser.append(ch);
        break;

    case XmlMode::DESCRIPTION: // static line
        m_staticRoomDesc += normalizeStringCopy(m_stringBuffer.simplified().append("\n"));
        if (!m_gratuitous) {
            toUser.append(ch);
        }
        break;

    case XmlMode::EXITS:
        m_exits += m_stringBuffer;
        if (!m_exitsReady) {
            m_exitsReady = true;
            m_descriptionReady = true;
        }
        break;

    case XmlMode::PROMPT:
        emit sendPromptLineEvent(normalizeStringCopy(m_stringBuffer).toLatin1());
        if (!m_exitsReady) {     // fixes compact mode
            m_exitsReady = true; // we finished read desc mode
            m_descriptionReady = true;
            if (config.mumeNative.emulatedExits) {
                emulateExits(m_move);
            }
        }
        if (m_descriptionReady) {
            parsePrompt(normalizeStringCopy(m_stringBuffer));
            move();
        }

        toUser.append(ch);
        break;

    case XmlMode::HEADER:
        // REVISIT: Why do this here? Can't we move this logic to </room>
        // We should get rid of m_readingRoomDesc
        if (!m_exitsReady) {
            m_descriptionReady = true;
        }
        toUser.append(ch);
        break;

    case XmlMode::TERRAIN:
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

    if (m_roomName == emptyString || // blindness
        Patterns::matchNoDescriptionPatterns(
            m_roomName)) { // non standard end of description parsed (fog, dark or so ...)
        m_roomName = nullString;
        m_dynamicRoomDesc = nullString;
        m_staticRoomDesc = nullString;
    }

    const auto emitEvent = [this]() {
        auto ev = ParseEvent::createEvent(m_move,
                                          m_roomName,
                                          m_dynamicRoomDesc,
                                          m_staticRoomDesc,
                                          m_exitsFlags,
                                          m_promptFlags,
                                          m_connectedRoomFlags);
        emit event(SigParseEvent{ev});
    };

    if (queue.isEmpty()) {
        emitEvent();
    } else {
        const CommandIdType c = queue.dequeue();
        // Ignore scouting unless it forced movement via a one-way
        if (c != CommandIdType::SCOUT
            || (c == CommandIdType::SCOUT && m_move != CommandIdType::LOOK)) {
            emit showPath(queue, false);
            emitEvent();
            if (c != m_move) {
                queue.clear();
            }
        }
    }
    m_move = CommandIdType::LOOK;
}

void MumeXmlParser::parseMudCommands(const QString &str)
{
    // REVISIT: Replace this if/else tree with a smarter data structure
    // REVISIT: Utilize the XML tag for more context?
    switch (str.at(0).toLatin1()) {
    case 'Y':
        if (str.startsWith("Your head stops stinging.")) {
            emit sendCharacterAffectEvent(CharacterAffect::BASHED, false);
            return;

        } else if (str.startsWith("You feel a cloak of blindness dissolve.")
                   || str.startsWith(
                          "Your head stops spinning and you can see clearly again.") // flash powder
        ) {
            emit sendCharacterAffectEvent(CharacterAffect::BLIND, false);
            return;
        } else if (str.startsWith("You have been blinded!")
                   || str.startsWith("An extremely bright flash of light stuns you!") // flash powder
        ) {
            emit sendCharacterAffectEvent(CharacterAffect::BLIND, true);
            return;
        } else if (str.startsWith("You feel very sleepy... zzzzzz")) {
            emit sendCharacterPositionEvent(CharacterPosition::SLEEPING);
            emit sendCharacterAffectEvent(CharacterAffect::SLEPT, true);
            return;
        } else if (str.startsWith("You feel less tired.")) {
            emit sendCharacterAffectEvent(CharacterAffect::SLEPT, false);
            return;
        } else if (str.startsWith(
                       "Your body turns numb as the poison speeds to your brain!") // Arachnia
                   || str.startsWith("You feel bad.")                              // Generic poison
                   || str.startsWith("You feel very sick.")                        // Disease?
                   || str.startsWith("You suddenly feel a terrible headache!")     // Psylonia
                   || str.startsWith("You feel sleepy.")                           // Psylonia tick
                   || str.startsWith("Your limbs are becoming cold and heavy,"     // Psylonia tick
                                     " your eyelids close.")) {
            emit sendCharacterAffectEvent(CharacterAffect::POISONED, true);
            return;
        } else if (str.startsWith("You are dead!")) {
            queue.clear();
            emit showPath(queue, true);
            emit releaseAllPaths();
            markCurrentCommand();
            emit sendCharacterPositionEvent(CharacterPosition::DEAD);
            return;
        } else if (str.startsWith("You failed to climb")) {
            if (!queue.isEmpty())
                queue.dequeue();
            queue.prepend(CommandIdType::NONE); // REVISIT: Do we need this?
            emit showPath(queue, true);
            return;
        } else if (str.startsWith("You flee head")) {
            queue.enqueue(CommandIdType::LOOK);
            return;
        } else if (str.startsWith("You follow")) {
            queue.enqueue(CommandIdType::LOOK);
            return;
        } else if (str.startsWith("You need to swim to go there.")
                   || str.startsWith("You cannot ride there.")
                   || str.startsWith("You are too exhausted.")
                   || str.startsWith("You are too exhausted to ride.")
                   || str.startsWith("Your mount refuses to follow your orders!")
                   || str.startsWith("You failed swimming there.")
                   || str.startsWith("You can't go into deep water!")
                   || str.startsWith("You unsuccessfully try to break through the ice.")
                   || str.startsWith("Your boat cannot enter this place.")) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;
        } else if (str.startsWith("You quietly scout")) {
            queue.prepend(CommandIdType::SCOUT);
            return;
        } else if (str.startsWith("You go to sleep.")
                   || str.startsWith("You are already sound asleep.")) {
            emit sendCharacterPositionEvent(CharacterPosition::SLEEPING);
            return;
        } else if (str.startsWith("You wake, and sit up.") || str.startsWith("You sit down.")
                   || str.startsWith("You stop resting and sit up.")
                   || str.startsWith("You're sitting already.")) {
            emit sendCharacterPositionEvent(CharacterPosition::SITTING);
            return;
        } else if (str.startsWith("You rest your tired bones.")
                   || str.startsWith("You sit down and rest your tired bones.")
                   || str.startsWith("You are already resting.")) {
            emit sendCharacterPositionEvent(CharacterPosition::RESTING);
            return;
        } else if (str.startsWith("You stop resting and stand up.")
                   || str.startsWith("You stand up.")
                   || str.startsWith("You are already standing.")) {
            emit sendCharacterPositionEvent(CharacterPosition::STANDING);
            return;
        } else if (str.startsWith("You are incapacitated and will slowly die, if not aided.")
                   || str.startsWith("You are in a pretty bad shape, unable to do anything!")
                   || str.startsWith(
                          "You're stunned and will probably die soon if no-one helps you.")
                   || str.startsWith("You are mortally wounded and will die soon if not aided.")) {
            emit sendCharacterPositionEvent(CharacterPosition::INCAPACITATED);
            return;
        } else if (str.startsWith("You bleed from open wounds.")) {
            emit sendCharacterAffectEvent(CharacterAffect::BLEEDING, true);
            return;
        } else if (str.startsWith("You begin to feel hungry.")
                   || str.startsWith("You are hungry.")) {
            emit sendCharacterAffectEvent(CharacterAffect::HUNGRY, true);
            return;
        } else if (str.startsWith("You begin to feel thirsty.")
                   || str.startsWith("You are thirsty.")) {
            emit sendCharacterAffectEvent(CharacterAffect::THIRSTY, true);
            return;
        } else if (str.startsWith("You do not feel thirsty anymore.")
                   || str.startsWith("You feel less thirsty.") // create water
        ) {
            emit sendCharacterAffectEvent(CharacterAffect::THIRSTY, false);
            return;
        } else if (str.startsWith("You are full.")) {
            emit sendCharacterAffectEvent(CharacterAffect::HUNGRY, false);
            return;
        } else if (str.startsWith("You can feel the broken bones within you heal "
                                  "and reshape themselves") // cure critic
        ) {
            emit sendCharacterAffectEvent(CharacterAffect::BLEEDING, false);
            return;
        }
        break;
    case 'T':
        if (str.startsWith("The current time is")) {
            m_mumeClock->parseClockTime(str);
            return;

            // The door
        } else if (str.endsWith("seems to be closed.")
                   // The (a|de)scent
                   || str.endsWith("is too steep, you need to climb to go there.")) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;

        } else if (str.startsWith("The venom enters your body!")        // Venom
                   || str.startsWith("The venom runs into your veins!") // Venom tick
        ) {
            emit sendCharacterAffectEvent(CharacterAffect::POISONED, true);
            return;
        }
        break;
    case 'A':
        if (str.startsWith("Alas, you cannot go that way...")
            // A pack horse
            || str.endsWith("is too exhausted.")) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;

        } else if (str.startsWith("A warm feeling runs through your body, you feel better.")) {
            // Remove poison <magic>
            emit sendCharacterAffectEvent(CharacterAffect::POISONED, false);
            return;

        } else if (str.startsWith("A warm feeling fills your body.")) {
            // Heal <magic>
            emit sendCharacterAffectEvent(CharacterAffect::POISONED, false);
            return;
        } else if (str.startsWith("A hot flush overwhelms your brain and makes you dizzy.")) {
            emit sendCharacterAffectEvent(CharacterAffect::POISONED, true);
            return;
        }
        break;

    case 'N':
        if (str.startsWith("No way! You are fighting for your life!")
            || str.startsWith("Nah... You feel too relaxed to do that.")) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;
        }
        break;
    case 'M':
        if (str.startsWith("Maybe you should get on your feet first?")) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;
        }
        break;
    case 'I':
        if (str.startsWith("In your dreams, or what?")) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;
        }
        break;
    case 'Z':
        // ZBLAM!
        if (str.endsWith("doesn't want you riding him anymore.")    // pack horse / pony
            || str.endsWith("doesn't want you riding her anymore.") // donkey
            || str.endsWith("doesn't want you riding it anymore.")  // hungry warg
        ) {
            if (!queue.isEmpty())
                queue.dequeue();
            emit showPath(queue, true);
            return;
        }
        break;
    case '-':
        if (str.startsWith("- poison (type: poison).")) {
            emit sendCharacterAffectEvent(CharacterAffect::POISONED, true);
            return;
        } else if (!str.startsWith("- a light wound") && !str.endsWith("bound)")
                   && str.contains("wound at the")) {
            emit sendCharacterAffectEvent(CharacterAffect::BLEEDING, true);
            return;
        }
    };

    if (str.endsWith("of the Third Age.")) {
        m_mumeClock->parseMumeTime(str);
        return;
    }

    // REVISIT: Move this to only be detected on <damage> XML tag?
    // What about custom mob bashes?
    if (str.endsWith("sends you sprawling with a powerful bash.")          // bash
        || str.endsWith("leaps at your throat and sends you sprawling.")   // cave-lion
        || str.endsWith("whips its tail around, and sends you sprawling!") // cave-worm
        || str.endsWith("sends you sprawling.")                            // various trees
    ) {
        emit sendCharacterAffectEvent(CharacterAffect::BASHED, true);
        return;
    }

    // Certain weather events happen on ticks
    if (m_readWeatherTag) {
        m_readWeatherTag = false;
        m_mumeClock->parseWeather(str);
        return;
    }
}
