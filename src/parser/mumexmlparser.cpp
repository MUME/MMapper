// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "mumexmlparser.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/CaseUtils.h"
#include "../global/Consts.h"
#include "../global/Diff.h"
#include "../global/PrintUtils.h"
#include "../global/TextUtils.h"
#include "../global/entities.h"
#include "../global/logging.h"
#include "../global/parserutils.h"
#include "../global/progresscounter.h"
#include "../map/ExitsFlags.h"
#include "../map/ParseTree.h"
#include "../map/PromptFlags.h"
#include "../map/parseevent.h"
#include "../mapdata/mapdata.h"
#include "../pandoragroup/mmapper2group.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/telnetfilter.h"
#include "abstractparser.h"
#include "patterns.h"

#include <cctype>
#include <list>
#include <optional>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QString>

using namespace char_consts;

namespace { // anonymous
void decodeXmlEntities(QString &s)
{
    s = entities::decode(entities::EncodedString{s});
}

void encodeXmlEntities(QString &s)
{
    s = entities::encode(entities::DecodedString{s});
}

NODISCARD QString getQString(const charset::conversion::Utf16StringBuilder &builder)
{
    return QString::fromStdU16String(builder.str());
}

void appendCodepoint(QString &qs, char32_t c)
{
    if ((false)) {
        qs += QString::fromStdU32String(std::u32string{c, 1});
    } else {
        charset::conversion::Utf16StringBuilder builder;
        builder += c;
        auto sv = builder.get_string_view();
        qs += QStringView{sv.data(), static_cast<int>(sv.size())};
    }
}

} // namespace

MumeXmlParser::MumeXmlParser(MapData &md,
                             MumeClock &mc,
                             ProxyParserApi proxy,
                             GroupManagerApi group,
                             CTimers &timers,
                             QObject *parent)
    : AbstractParser(md, mc, std::move(proxy), std::move(group), timers, parent)
{}

MumeXmlParser::~MumeXmlParser() = default;

void MumeXmlParser::slot_parseNewMudInput(const TelnetData &data)
{
    const bool isPrompt = data.type == TelnetDataEnum::Prompt;
    const bool isTwiddlers = data.type == TelnetDataEnum::Delay;
    if (isTwiddlers) {
        m_lastPrompt = QString::fromUtf8(data.line.getQByteArray());
        if (getConfig().parser.removeXmlTags) {
            decodeXmlEntities(m_lastPrompt);
        }
    }
    parse(data, isPrompt || isTwiddlers);
}

void MumeXmlParser::parse(const TelnetData &data, const bool isGoAhead)
{
    m_lineToUser.clear();
    m_lineFlags.remove(LineFlagEnum::NONE);

    std::string_view utf8 = mmqt::toStdStringViewRaw(data.line.getQByteArray());
    charset::foreach_codepoint_utf8(utf8, [this](const char32_t c) {
        if (m_readingTag) {
            if (c == char_consts::C_GREATER_THAN) {
                // send tag
                if (!m_tempTag.isEmpty()) {
                    std::ignore = element(m_tempTag);
                }

                m_tempTag.clear();

                m_readingTag = false;
                return;
            }
            appendCodepoint(m_tempTag, c);

        } else {
            if (c == char_consts::C_LESS_THAN) {
                m_lineToUser.append(characters(m_tempCharacters));
                m_tempCharacters.clear();

                m_readingTag = true;
                return;
            }
            appendCodepoint(m_tempCharacters, c);
        }
    });

    if (!m_readingTag) {
        m_lineToUser.append(characters(m_tempCharacters));
        m_tempCharacters.clear();
    }
    if (!m_lineToUser.isEmpty()) {
        sendToUser(m_lineToUser, isGoAhead);

        // Simplify the output and run actions
        QString temp = m_lineToUser;
        if (!getConfig().parser.removeXmlTags) {
            decodeXmlEntities(temp);
        }
        QString tempStr = temp;
        tempStr = normalizeStringCopy(tempStr.trimmed());
        parseMudCommands(tempStr);
    }
}

bool MumeXmlParser::element(const QString &line)
{
    using namespace char_consts;
    const int length = line.length();

    // REVISIT: Merge this logic with the state machine in parse()
    const auto attrs = [&line]() {
        std::list<std::pair<std::string, std::string>> attributes;

        std::ostringstream os;
        std::optional<std::string> key;

        XmlAttributeStateEnum state = XmlAttributeStateEnum::ELEMENT;
        const auto makeAttribute = [&key, &os, &attributes, &state]() {
            assert(key.has_value());
            // REVISIT: Translate XML entities into text
            attributes.emplace_back(key.value(), os.str());
            key.reset();
            os.str(std::string());
            state = XmlAttributeStateEnum::ATTRIBUTE;
        };
        for (const QChar qc : line) {
            if (qc.unicode() >= 256) {
                continue;
            }
            const char c = qc.toLatin1();

            switch (state) {
            case XmlAttributeStateEnum::ELEMENT:
                if (ascii::isSpace(c)) {
                    state = XmlAttributeStateEnum::ATTRIBUTE;
                } else {
                    continue;
                }
                break;
            case XmlAttributeStateEnum::ATTRIBUTE:
                if (ascii::isSpace(c)) {
                    continue;
                } else if (c == C_EQUALS) {
                    key = os.str();
                    os.str(std::string());
                    state = XmlAttributeStateEnum::EQUALS;
                } else {
                    os << c;
                }
                break;
            case XmlAttributeStateEnum::EQUALS:
                if (ascii::isSpace(c)) {
                    continue;
                } else if (c == C_SQUOTE) {
                    state = XmlAttributeStateEnum::SINGLE_QUOTED_VALUE;
                } else if (c == C_DQUOTE) {
                    state = XmlAttributeStateEnum::DOUBLE_QUOTED_VALUE;
                } else {
                    os << c;
                    state = XmlAttributeStateEnum::UNQUOTED_VALUE;
                }
                break;
            case XmlAttributeStateEnum::UNQUOTED_VALUE:
                // Note: This format is not valid according to the W3C XML standard
                if (ascii::isSpace(c) || c == C_SLASH) {
                    makeAttribute();
                } else {
                    os << c;
                }
                break;
            case XmlAttributeStateEnum::SINGLE_QUOTED_VALUE:
                if (c == C_SQUOTE) {
                    makeAttribute();
                } else {
                    os << c;
                }
                break;
            case XmlAttributeStateEnum::DOUBLE_QUOTED_VALUE:
                if (c == C_DQUOTE) {
                    makeAttribute();
                } else {
                    os << c;
                }
                break;
            }
        }
        if (key.has_value()) {
            makeAttribute();
        }
        return attributes;
    }();

    switch (m_xmlMode) {
    case XmlModeEnum::NONE:
        if (length > 0) {
            switch (line.front().unicode()) {
            case C_SLASH:
                if (line.startsWith("/snoop")) {
                    m_descriptionReady = false;
                    m_lineFlags.remove(LineFlagEnum::SNOOP);

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
                    m_descriptionReady = false;
                    m_exitsReady = false;
                    m_exits = nullString;
                    m_stringBuffer = nullString;
                    m_roomContents.reset();
                    m_lineFlags.insert(LineFlagEnum::ROOM);
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
                }
                break;
            }
        }
        break;

    case XmlModeEnum::ROOM:
        if (length > 0) {
            switch (line.front().unicode()) {
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
                    m_roomContents = mmqt::makeRoomContents(m_stringBuffer);
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
            switch (line.front().unicode()) {
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
            switch (line.front().unicode()) {
            case C_SLASH:
                if (line.startsWith("/exits")) {
                    std::ostringstream os;
                    parseExits(os);
                    m_lineToUser.append(mmqt::toQStringUtf8(os.str()));
                    m_exitsReady = true;
                    m_lineFlags.remove(LineFlagEnum::EXITS);
                    m_xmlMode = (m_lineFlags.contains(LineFlagEnum::ROOM) ? XmlModeEnum::ROOM
                                                                          : XmlModeEnum::NONE);
                }
                break;
            }
        }
        break;
    case XmlModeEnum::PROMPT:
        if (length > 0) {
            switch (line.front().unicode()) {
            case C_SLASH:
                if (line.startsWith("/prompt")) {
                    m_xmlMode = XmlModeEnum::NONE;
                    m_lineFlags.remove(LineFlagEnum::PROMPT);
                    m_overrideSendPrompt = false;

                    const QString copy = normalizeStringCopy(m_lastPrompt);
                    sendPromptLineEvent(copy);

                    const auto &config = getConfig();
                    if (!config.parser.removeXmlTags) {
                        encodeXmlEntities(m_lastPrompt);
                        m_lastPrompt = "<prompt>" + m_lastPrompt + "</prompt>";
                    }

                    if (m_descriptionReady) {
                        if (!m_exitsReady && config.mumeNative.emulatedExits) {
                            m_exitsReady = true;
                            std::ostringstream os;
                            {
                                AnsiOstream aos{os};
                                emulateExits(aos, m_move);
                            }
                            sendToUser(os.str());
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
            switch (line.front().unicode()) {
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
            switch (line.front().unicode()) {
            case C_SLASH:
                if (line.startsWith("/header")) {
                    m_xmlMode = XmlModeEnum::ROOM;
                    m_lineFlags.remove(LineFlagEnum::HEADER);
                }
                break;
            }
        }
        break;
    }

    if (!getConfig().parser.removeXmlTags) {
        m_lineToUser.append(C_LESS_THAN).append(line).append(C_GREATER_THAN);
    }

    return true;
}

QString MumeXmlParser::characters(QString &ch)
{
    if (ch.isEmpty()) {
        return ch;
    }

    // replace > and < chars
    decodeXmlEntities(ch);

    const auto &config = getConfig();

    QString toUser;

    const XmlModeEnum mode = [this]() -> XmlModeEnum {
        if (m_lineFlags.isPrompt()) {
            return XmlModeEnum::PROMPT;
        } else if (m_lineFlags.isExits()) {
            return XmlModeEnum::EXITS;
        } else if (m_lineFlags.isName()) {
            return XmlModeEnum::NAME;
        } else if (m_lineFlags.isDescription()) {
            return XmlModeEnum::DESCRIPTION;
        } else if (m_lineFlags.isRoom()) {
            return XmlModeEnum::ROOM;
        }
        return m_xmlMode;
    }();

    switch (mode) {
    case XmlModeEnum::NONE: // non room info
        if (ch.isEmpty()) { // standard end of description parsed
            if (m_descriptionReady && !m_exitsReady && config.mumeNative.emulatedExits) {
                m_exitsReady = true;
                std::ostringstream os;
                {
                    AnsiOstream aos{os};
                    emulateExits(aos, m_move);
                }
                sendToUser(os.str());
            }
        } else {
            m_lineFlags.insert(LineFlagEnum::NONE);
        }
        toUser.append(ch);
        break;

    case XmlModeEnum::ROOM: // dynamic line
        if (!m_descriptionReady) {
            m_stringBuffer += ch;
        }
        toUser.append(ch);
        break;

    case XmlModeEnum::NAME:
        toUser.append(ch);
        break;

    case XmlModeEnum::DESCRIPTION: // static line
        if (!m_gratuitous) {
            toUser.append(ch);
        }
        break;

    case XmlModeEnum::EXITS:
        m_exits += ch;
        break;

    case XmlModeEnum::PROMPT:
        // Store prompts in case an internal command is executed
        m_lastPrompt += ch;
        toUser.append(ch);
        break;

    case XmlModeEnum::HEADER:
    case XmlModeEnum::TERRAIN:
    default:
        toUser.append(ch);
        break;
    }

    if (!getConfig().parser.removeXmlTags) {
        encodeXmlEntities(toUser);
    }
    return toUser;
}

void MumeXmlParser::setMove(const CommandEnum move)
{
    m_move = move;
    m_expectedMove.reset();

    const auto dir = getDirection(move);
    if (!isNESWUD(dir)) {
        return;
    }

    auto &map = m_mapData;
    auto pHere = map.getCurrentRoom();
    if (!pHere) {
        return;
    }

    const auto &here = *pHere;
    if (false /*!here.isUpToDate()*/) {
        return;
    }

    const auto &ex = here.getExit(dir);
    if (!ex.exitIsExit()) {
        return;
    }

    const auto &set = ex.getOutgoingSet();
    if (set.size() != 1) {
        return;
    }

    const RoomId id = set.first();
    const auto &pThere = map.findRoomHandle(id);
    if (!pThere) {
        return;
    }

    const auto &there = *pThere;
    if (false /*!there.isUpToDate() */ || there.getName().empty()
        || there.getDescription().empty()) {
        return;
    }

    const Map &mm3Map = map.getCurrentMap();
    if (!mm3Map.hasUniqueNameDesc(id)) {
        return;
    }

    m_expectedMove = id;
}

// REVISIT: This is likely to be a performance problem.
NODISCARD static bool isMostlyTheSame(const RoomDesc &a, const RoomDesc &b, const double cutoff)
{
    assert(std::isfinite(cutoff)
           && isClamped(cutoff, std::nextafter(50.0, 51.0), std::nextafter(100.0, 99.0)));

    if (a.empty() || b.empty()) {
        return false;
    }

    struct NODISCARD MyDiff final : public diff::Diff<StringView>
    {
    public:
        size_t removedBytes = 0;
        size_t addedBytes = 0;
        size_t commonBytes = 0;

    private:
        void virt_report(diff::SideEnum side, const Range &r) final
        {
            switch (side) {
            case diff::SideEnum::A:
                for (auto x : r) {
                    removedBytes += x.size();
                }
                break;
            case diff::SideEnum::B:
                for (auto x : r) {
                    addedBytes += x.size();
                }
                break;
            case diff::SideEnum::Common:
                for (auto x : r) {
                    commonBytes += x.size();
                }
                break;
            }
        }
    };

    const std::vector<StringView> aWords = StringView{a.getStdStringViewUtf8()}.getWords();
    const std::vector<StringView> bWords = StringView{b.getStdStringViewUtf8()}.getWords();

    MyDiff diff;
    diff.compare(MyDiff::Range::fromVector(aWords), MyDiff::Range::fromVector(bWords));

    const auto max_change = std::max(diff.removedBytes, diff.addedBytes);
    const auto min_change = std::min(diff.removedBytes, diff.addedBytes);
    const auto total = min_change + diff.commonBytes;

    if (total < 20 || max_change >= total) {
        return false;
    }

    const auto ratio = static_cast<double>(total - max_change) / static_cast<double>(total);
    const auto percent = ratio * 100.0;
    const auto rounded = static_cast<double>(std::lround(percent * 10.0)) * 0.1;

    MMLOG() << "[XML parser] Score: " << rounded << "% word match";
    return rounded >= cutoff;
}

void MumeXmlParser::maybeUpdate(RoomId expectedId, const ParseEvent &ev)
{
    const auto &evname = ev.getRoomName();
    const auto &evdesc = ev.getRoomDesc();

    if (evname.empty() || evdesc.empty()) {
        return;
    }

    const auto &pExpected = m_mapData.findRoomHandle(expectedId);
    if (!pExpected) {
        return;
    }

    const auto &expected = *pExpected;
    const auto &name = expected.getName();
    const auto &desc = expected.getDescription();

    if (name == evname && evdesc == desc) {
        // This is what we want in 99+% of the cases
        return;
    }

    if (false /*!expected.isUpToDate()*/ || name.empty() || desc.empty()) {
        // Not our problem.
        return;
    }

    auto update = [this, expectedId](const std::string_view what, const auto &field) {
        const auto &pRoom = m_mapData.findRoomHandle(expectedId);
        if (!pRoom) {
            return;
        }

        const auto extId = pRoom->getIdExternal();
        const auto change = room_change_types::ModifyRoomFlags{expectedId,
                                                               field,
                                                               FlagModifyModeEnum::ASSIGN};
        if (!m_mapData.applySingleChange(Change{change})) {
            MMLOG() << "[XML parser] Ooops. The update failed?";
            return;
        }

        MMLOG() << "[XML parser] Auto-updated room " << what << " for room " << extId.value()
                << ".";

        // REVISIT: Can we eliminate the Qt logging?
        const auto msg = QString("Auto-updated %1 for room %2.")
                             .arg(mmqt::toQStringUtf8(what))
                             .arg(extId.value());
        log("MumeXmlParser", msg);
    };

    if (name == evname && isMostlyTheSame(desc, evdesc, 80.0)) {
        // update the desc for a unique name? That's a no-brainer.
        MMLOG() << "[XML parser] Exact name match.";
        update("description", evdesc);
        return;
    }

    if (desc == evdesc) {
        // Update the name for what used to be a unique name+desc, and the desc still matches...
        // should we still see if it's a similar name at least?
        MMLOG() << "[XML parser] Exact description match.";
        update("name", evname);
        return;
    }

    if (isMostlyTheSame(desc, evdesc, 90.0)) {
        MMLOG() << "[XML parser] Good enough description match.";
        update("description", evdesc);
        return;
    }
}

void MumeXmlParser::move()
{
    m_descriptionReady = false;

    const auto expectedMove = std::exchange(m_expectedMove, {});

    // non-standard end of description parsed (blindness, fog, dark or so ...)
    if (m_roomName.has_value()
        && Patterns::matchNoDescriptionPatterns(m_roomName.value().toQString())) {
        m_roomName.reset();
        m_roomDesc.reset();
        m_roomContents.reset();
    }

    const auto emitEvent = [this, expectedMove]() {
        auto ev = ParseEvent::createSharedEvent(m_move,
                                                m_serverId,
                                                m_roomName.value_or(RoomName{}),
                                                m_roomDesc.value_or(RoomDesc{}),
                                                m_roomContents.value_or(RoomContents{}),
                                                m_exitIds.value_or(ServerExitIds{}),
                                                m_terrain,
                                                m_exitsFlags,
                                                m_promptFlags,
                                                m_connectedRoomFlags);

        // TODO: only do this in PLAY mode.
        if (expectedMove) {
            maybeUpdate(expectedMove.value(), *ev);
        }

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
                MMLOG() << "[XML parser] move " << getUppercase(m_move) << " doesn't match command "
                        << getUppercase(c);
                m_queue.clear();
            }
        }
    }

    setMove(CommandEnum::LOOK);
}

void MumeXmlParser::parseMudCommands(const QString &str)
{
    // REVISIT: Add XML tag-based actions that match on a given LineFlag
    const auto stdString = mmqt::toStdStringUtf8(str);
    if (evalActionMap(StringView{stdString})) {
        return;
    }
}
