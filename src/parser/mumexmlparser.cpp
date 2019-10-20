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

MumeXmlParser::MumeXmlParser(MapData *md, MumeClock *mc, ProxyParserApi proxy, QObject *parent)
    : AbstractParser(md, mc, proxy, parent)
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
    case TelnetDataEnum::DELAY: // Twiddlers
    case TelnetDataEnum::MENU_PROMPT:
    case TelnetDataEnum::LOGIN:
    case TelnetDataEnum::LOGIN_PASSWORD:
        m_lastPrompt = data.line;
        stripXmlEntities(m_lastPrompt);
        parse(data);
        break;
    case TelnetDataEnum::SPLIT:
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
    case TelnetDataEnum::LFCR:
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

void MumeXmlParser::parse(const IncomingData &data)
{
    const QByteArray &line = data.line;
    m_lineToUser.clear();

    for (int index = 0; index < line.size(); index++) {
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
        const auto isGoAhead = [](const TelnetDataEnum type) -> bool {
            switch (type) {
            case TelnetDataEnum::DELAY:
            case TelnetDataEnum::LOGIN:
            case TelnetDataEnum::LOGIN_PASSWORD:
            case TelnetDataEnum::MENU_PROMPT:
            case TelnetDataEnum::PROMPT:
                return true;
            case TelnetDataEnum::CRLF:
            case TelnetDataEnum::LFCR:
            case TelnetDataEnum::LF:
            case TelnetDataEnum::SPLIT:
            case TelnetDataEnum::UNKNOWN:
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
    const XmlModeEnum lastMode = m_xmlMode;

    switch (m_xmlMode) {
    case XmlModeEnum::NONE:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/xml")) {
                    sendToUser("[MMapper] Mapper cannot function without XML mode\n");
                    m_queue.clear();
                }
                break;
            case 'p':
                if (line.startsWith("prompt")) {
                    m_xmlMode = XmlModeEnum::PROMPT;
                }
                break;
            case 'e':
                if (line.startsWith("exits")) {
                    m_exits = nullString; // Reset string since payload can be from the 'exit' command
                    m_xmlMode = XmlModeEnum::EXITS;
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
                            };
                        }
                        break;
                    case '/':
                        m_move = CommandEnum::NONE;
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
                    m_xmlMode = XmlModeEnum::NONE;
                } else if (line.startsWith("snoop")) {
                    m_readSnoopTag = true;
                }
                break;
            case 'h':
                if (line.startsWith("header")) {
                    m_xmlMode = XmlModeEnum::HEADER;
                }
                break;
            }
        };
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
                }
                break;
            case 'd':
                if (line.startsWith("description")) {
                    m_xmlMode = XmlModeEnum::DESCRIPTION;
                    // might be empty but valid description
                    m_staticRoomDesc = RoomStaticDesc{""};
                }
                break;
            case 't': // terrain tag only comes up in blindness or fog
                if (line.startsWith("terrain")) {
                    m_xmlMode = XmlModeEnum::TERRAIN;
                }
                break;

            case '/':
                if (line.startsWith("/room")) {
                    m_xmlMode = XmlModeEnum::NONE;
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
        }
        break;
    case XmlModeEnum::DESCRIPTION:
        if (length > 0) {
            switch (line.at(0)) {
            case '/':
                if (line.startsWith("/description")) {
                    m_xmlMode = XmlModeEnum::ROOM;
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
                    parseExits();
                    m_exitsReady = true;
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
                }
                break;
            }
        }
        break;
    }

    if (!getConfig().parser.removeXmlTags) {
        m_lineToUser.append(lessThanChar).append(line).append(greaterThanChar);
    }

    if (lastMode == XmlModeEnum::PROMPT) {
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
    case XmlModeEnum::NONE: // non room info
        m_stringBuffer = normalizeStringCopy(m_stringBuffer.trimmed());
        if (m_stringBuffer.isEmpty()) { // standard end of description parsed
            if (m_descriptionReady && !m_exitsReady && config.mumeNative.emulatedExits) {
                m_exitsReady = true;
                emulateExits(m_move);
            }
        } else {
            parseMudCommands(m_stringBuffer);
        }
        if (m_readSnoopTag) {
            if (m_descriptionReady) {
                m_promptFlags.reset(); // Don't trust god prompts
                m_queue.enqueue(m_move);
                pathChanged();
                move();
                m_readSnoopTag = false;
            }
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
        emit sendPromptLineEvent(normalizeStringCopy(m_stringBuffer).toLatin1());
        if (!m_exitsReady && config.mumeNative.emulatedExits) {
            m_exitsReady = true;
            emulateExits(m_move);
        }
        if (m_descriptionReady) {
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
    // REVISIT: Replace this if/else tree with a smarter data structure
    // REVISIT: Utilize the XML tag for more context?
    switch (str.at(0).toLatin1()) {
    case 'Y':
        if (str.startsWith("Your head stops stinging.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::BASHED, false);
            return;

        } else if (str.startsWith("You feel a cloak of blindness dissolve.")
                   || str.startsWith(
                          "Your head stops spinning and you can see clearly again.") // flash powder
        ) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::BLIND, false);
            return;
        } else if (str.startsWith("You have been blinded!")
                   || str.startsWith("An extremely bright flash of light stuns you!") // flash powder
        ) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::BLIND, true);
            return;
        } else if (str.startsWith("You feel very sleepy... zzzzzz")) {
            emit sendCharacterPositionEvent(CharacterPositionEnum::SLEEPING);
            emit sendCharacterAffectEvent(CharacterAffectEnum::SLEPT, true);
            return;
        } else if (str.startsWith("You feel less tired.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::SLEPT, false);
            return;
        } else if (str.startsWith(
                       "Your body turns numb as the poison speeds to your brain!") // Arachnia
                   || str.startsWith("You feel bad.")                              // Generic poison
                   || str.startsWith("You feel very sick.")                        // Disease?
                   || str.startsWith("You suddenly feel a terrible headache!")     // Psylonia
                   || str.startsWith("You feel sleepy.")                           // Psylonia tick
                   || str.startsWith("Your limbs are becoming cold and heavy,"     // Psylonia tick
                                     " your eyelids close.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, true);
            return;
        } else if (str.startsWith("You are dead!")) {
            m_queue.clear();
            pathChanged();
            emit releaseAllPaths();
            markCurrentCommand();
            emit sendCharacterPositionEvent(CharacterPositionEnum::DEAD);
            return;
        } else if (str.startsWith("You failed to climb")) {
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            m_queue.prepend(CommandEnum::NONE); // REVISIT: Do we need this?
            pathChanged();
            return;
        } else if (str.startsWith("You flee head")) {
            m_queue.enqueue(CommandEnum::LOOK);
            return;
        } else if (str.startsWith("You follow")) {
            m_queue.enqueue(CommandEnum::LOOK);
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
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;
        } else if (str.startsWith("You quietly scout")) {
            m_queue.prepend(CommandEnum::SCOUT);
            return;
        } else if (str.startsWith("You go to sleep.")
                   || str.startsWith("You are already sound asleep.")) {
            emit sendCharacterPositionEvent(CharacterPositionEnum::SLEEPING);
            return;
        } else if (str.startsWith("You wake, and sit up.") || str.startsWith("You sit down.")
                   || str.startsWith("You stop resting and sit up.")
                   || str.startsWith("You're sitting already.")) {
            emit sendCharacterPositionEvent(CharacterPositionEnum::SITTING);
            return;
        } else if (str.startsWith("You rest your tired bones.")
                   || str.startsWith("You sit down and rest your tired bones.")
                   || str.startsWith("You are already resting.")) {
            emit sendCharacterPositionEvent(CharacterPositionEnum::RESTING);
            return;
        } else if (str.startsWith("You stop resting and stand up.")
                   || str.startsWith("You stand up.")
                   || str.startsWith("You are already standing.")) {
            emit sendCharacterPositionEvent(CharacterPositionEnum::STANDING);
            return;
        } else if (str.startsWith("You are incapacitated and will slowly die, if not aided.")
                   || str.startsWith("You are in a pretty bad shape, unable to do anything!")
                   || str.startsWith(
                          "You're stunned and will probably die soon if no-one helps you.")
                   || str.startsWith("You are mortally wounded and will die soon if not aided.")) {
            emit sendCharacterPositionEvent(CharacterPositionEnum::INCAPACITATED);
            return;
        } else if (str.startsWith("You bleed from open wounds.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::BLEEDING, true);
            return;
        } else if (str.startsWith("You begin to feel hungry.")
                   || str.startsWith("You are hungry.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::HUNGRY, true);
            return;
        } else if (str.startsWith("You begin to feel thirsty.")
                   || str.startsWith("You are thirsty.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::THIRSTY, true);
            return;
        } else if (str.startsWith("You do not feel thirsty anymore.")
                   || str.startsWith("You feel less thirsty.") // create water
        ) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::THIRSTY, false);
            return;
        } else if (str.startsWith("You are full.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::HUNGRY, false);
            return;
        } else if (str.startsWith("You can feel the broken bones within you heal "
                                  "and reshape themselves") // cure critic
        ) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::BLEEDING, false);
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
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;

        } else if (str.startsWith("The venom enters your body!")        // Venom
                   || str.startsWith("The venom runs into your veins!") // Venom tick
        ) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, true);
            return;
        }
        break;
    case 'A':
        if (str.startsWith("Alas, you cannot go that way...")
            // A pack horse
            || str.endsWith("is too exhausted.")) {
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;

        } else if (str.startsWith("A warm feeling runs through your body, you feel better.")) {
            // Remove poison <magic>
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, false);
            return;

        } else if (str.startsWith("A warm feeling fills your body.")) {
            // Heal <magic>
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, false);
            return;
        } else if (str.startsWith("A hot flush overwhelms your brain and makes you dizzy.")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, true);
            return;
        }
        break;

    case 'N':
        if (str.startsWith("No way! You are fighting for your life!")
            || str.startsWith("Nah... You feel too relaxed to do that.")) {
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;
        }
        break;
    case 'M':
        if (str.startsWith("Maybe you should get on your feet first?")) {
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;
        }
        break;
    case 'I':
        if (str.startsWith("In your dreams, or what?")) {
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;
        }
        break;
    case 'Z':
        // ZBLAM!
        if (str.endsWith("doesn't want you riding him anymore.")    // pack horse / pony
            || str.endsWith("doesn't want you riding her anymore.") // donkey
            || str.endsWith("doesn't want you riding it anymore.")  // hungry warg
        ) {
            if (!m_queue.isEmpty())
                m_queue.dequeue();
            pathChanged();
            return;
        }
        break;
    case '-':
        if (str.startsWith("- antidote")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, false);
            return;
        } else if (str.startsWith("- poison (type: poison).")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::POISONED, true);
            return;
        } else if (!str.startsWith("- a light wound") && !str.endsWith("bound)")
                   && str.contains("wound at the")) {
            emit sendCharacterAffectEvent(CharacterAffectEnum::BLEEDING, true);
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
        emit sendCharacterAffectEvent(CharacterAffectEnum::BASHED, true);
        return;
    }

    // Certain weather events happen on ticks
    if (m_readWeatherTag) {
        m_readWeatherTag = false;
        m_mumeClock->parseWeather(str);
        return;
    }
}
