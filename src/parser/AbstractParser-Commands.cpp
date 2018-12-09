/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#include "AbstractParser-Commands.h"

#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <QMessageLogContext>
#include <QtCore>

#include "../configuration/configuration.h"
#include "../global/StringView.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/enums.h"
#include "../mapdata/mmapper2room.h"
#include "Abbrev.h"
#include "AbstractParser-Utils.h"
#include "CommandId.h"
#include "DoorAction.h"
#include "abstractparser.h"

const Abbrev cmdBack{"back"};
const Abbrev cmdDirections{"dirs", 3};
const Abbrev cmdDoorHelp{"doorhelp", 5};
const Abbrev cmdGroupHelp{"grouphelp", 6};
const Abbrev cmdGroupKick{"gkick", 2};
const Abbrev cmdGroupLock{"glock", 2};
const Abbrev cmdGroupTell{"gtell", 2};
const Abbrev cmdHelp{"help", 2};
const Abbrev cmdMapHelp{"maphelp", 5};
const Abbrev cmdMarkCurrent{"markcurrent", 4};
const Abbrev cmdName{"name"};
const Abbrev cmdNote{"note"};
const Abbrev cmdRemoveDoorNames{"removedoornames"};
const Abbrev cmdSearch{"search", 3};
const Abbrev cmdSet{"set", 2};
const Abbrev cmdTime{"time", 2};
const Abbrev cmdTrollExit{"trollexit", 2};
const Abbrev cmdVote{"vote", 2};
const Abbrev cmdPDynamic{"pdynamic", 4};
const Abbrev cmdPStatic{"pstatic", 3};
const Abbrev cmdPNote{"pnote", 3};
const Abbrev cmdPrint{"print", 3};

Abbrev getParserCommandName(const DoorFlag x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case DoorFlag::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(HIDDEN, "hidden", 3);
        CASE3(NEED_KEY, "needkey", -1);
        CASE3(NO_BLOCK, "noblock", -1);
        CASE3(NO_BREAK, "nobreak", -1);
        CASE3(NO_PICK, "nopick", -1);
        CASE3(DELAYED, "delayed", 5);
        CASE3(CALLABLE, "callable", 4);
        CASE3(KNOCKABLE, "knockable", 5);
        CASE3(MAGIC, "magic", 3);
        CASE3(ACTION, "action", 3);
        CASE3(NO_BASH, "nobash", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomLightType x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomLightType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(UNDEFINED, "undefined", -1);
        CASE3(LIT, "lit", -1);
        CASE3(DARK, "dark", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomSundeathType x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomSundeathType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(UNDEFINED, "undefined", -1);
        CASE3(NO_SUNDEATH, "nosundeath", -1);
        CASE3(SUNDEATH, "sundeath", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomPortableType x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomPortableType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(UNDEFINED, "undefined", -1);
        CASE3(PORTABLE, "port", -1);
        CASE3(NOT_PORTABLE, "noport", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomRidableType x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomRidableType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(UNDEFINED, "undefined", -1);
        CASE3(RIDABLE, "ride", -1);
        CASE3(NOT_RIDABLE, "noride", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomAlignType x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomAlignType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(UNDEFINED, "undefined", -1);
        CASE3(GOOD, "good", -1);
        CASE3(NEUTRAL, "neutral", -1);
        CASE3(EVIL, "evil", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomMobFlag x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomMobFlag::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(RENT, "rent", -1);
        CASE3(SHOP, "shop", -1);
        CASE3(WEAPON_SHOP, "weaponshop", -1); // conflict with "weapon"
        CASE3(ARMOUR_SHOP, "armourshop", -1); // conflict with "armour"
        CASE3(FOOD_SHOP, "foodshop", -1);     // conflict with "food"
        CASE3(PET_SHOP, "petshop", 3);
        CASE3(GUILD, "guild", -1);
        CASE3(SCOUT_GUILD, "scoutguild", 5);
        CASE3(MAGE_GUILD, "mageguild", 4);
        CASE3(CLERIC_GUILD, "clericguild", 6);
        CASE3(WARRIOR_GUILD, "warriorguild", 7);
        CASE3(RANGER_GUILD, "rangerguild", 6);
        CASE3(AGGRESSIVE_MOB, "aggmob", -1);
        CASE3(QUEST_MOB, "questmob", -1);
        CASE3(PASSIVE_MOB, "passivemob", -1);
        CASE3(ELITE_MOB, "elitemob", -1);
        CASE3(SUPER_MOB, "smob", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomLoadFlag x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomLoadFlag::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(TREASURE, "treasure", -1);
        CASE3(ARMOUR, "armour", -1);
        CASE3(WEAPON, "weapon", -1);
        CASE3(WATER, "water", -1);
        CASE3(FOOD, "food", -1);
        CASE3(HERB, "herb", -1);
        CASE3(KEY, "key", -1);
        CASE3(MULE, "mule", -1);
        CASE3(HORSE, "horse", -1);
        CASE3(PACK_HORSE, "pack", -1);
        CASE3(TRAINED_HORSE, "trained", -1);
        CASE3(ROHIRRIM, "rohirrim", -1);
        CASE3(WARG, "warg", -1);
        CASE3(BOAT, "boat", -1);
        CASE3(ATTENTION, "attention", -1);
        CASE3(TOWER, "watch", -1);
        CASE3(CLOCK, "clock", -1);
        CASE3(MAIL, "mail", -1);
        CASE3(STABLE, "stable", -1);
        CASE3(WHITE_WORD, "whiteword", -1);
        CASE3(DARK_WORD, "darkword", -1);
        CASE3(EQUIPMENT, "equipment", -1);
    }
    return Abbrev{};
#undef CASE3
}

// NOTE: This isn't used by the parser (currently only used for filenames).
Abbrev getParserCommandName(const RoomTerrainType x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomTerrainType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(UNDEFINED, "undefined", -1);
        CASE3(INDOORS, "indoors", -1);
        CASE3(CITY, "city", -1);
        CASE3(FIELD, "field", -1);
        CASE3(FOREST, "forest", -1);
        CASE3(HILLS, "hills", -1);
        CASE3(MOUNTAINS, "mountains", -1);
        CASE3(SHALLOW, "shallow", -1);
        CASE3(WATER, "water", -1);
        CASE3(RAPIDS, "rapids", -1);
        CASE3(UNDERWATER, "underwater", -1);
        CASE3(ROAD, "road", -1);
        CASE3(BRUSH, "brush", -1);
        CASE3(TUNNEL, "tunnel", -1);
        CASE3(CAVERN, "cavern", -1);
        CASE3(DEATHTRAP, "deathtrap", -1);
    }
    return Abbrev{};
#undef CASE3
}

QByteArray getCommandName(const DoorActionType action)
{
#define CASE2(UPPER, s) \
    do { \
    case DoorActionType::UPPER: \
        return s; \
    } while (false)
    switch (action) {
        CASE2(OPEN, "open");
        CASE2(CLOSE, "close");
        CASE2(LOCK, "lock");
        CASE2(UNLOCK, "unlock");
        CASE2(PICK, "pick");
        CASE2(ROCK, "throw rock");
        CASE2(BASH, "bash");
        CASE2(BREAK, "cast 'break door'");
        CASE2(BLOCK, "cast 'block door'");

    case DoorActionType::NONE:
        break;
    }

    // REVISIT: use "look" ?
    return "";
#undef CASE2
}

Abbrev getParserCommandName(const DoorActionType action)
{
#define CASE3(UPPER, s, n) \
    do { \
    case DoorActionType::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (action) {
        CASE3(OPEN, "open", 2);
        CASE3(CLOSE, "close", 3);
        CASE3(LOCK, "lock", 3);
        CASE3(UNLOCK, "unlock", 3);
        CASE3(PICK, "pick", -1);
        CASE3(ROCK, "rock", -1);
        CASE3(BASH, "bash", -1);
        CASE3(BREAK, "break", -1);
        CASE3(BLOCK, "block", -1);
    case DoorActionType::NONE:
        break;
    }

    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const ExitFlag x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case ExitFlag::UPPER: \
        return Abbrev{s, n}; \
    } while (false)
    switch (x) {
        CASE3(DOOR, "door", -1);
        CASE3(EXIT, "exit", -1);
        CASE3(ROAD, "road", -1);
        CASE3(CLIMB, "climb", 3);
        CASE3(RANDOM, "random", 4);
        CASE3(SPECIAL, "special", 4);
        CASE3(NO_MATCH, "nomatch", -1);
        CASE3(FLOW, "flow", -1);
        CASE3(NO_FLEE, "noflee", -1);
        CASE3(DAMAGE, "damage", -1);
        CASE3(FALL, "fall", -1);
        CASE3(GUARDED, "guarded", 5);
    }
    return Abbrev{};
#undef CASE3
}

static bool isCommand(const QString &str, Abbrev abbrev)
{
    if (!abbrev)
        return false;

    auto view = StringView{str}.trim();
    if (view.isEmpty())
        return false;

    const auto word = view.takeFirstWord();
    return abbrev.matches(word);
}

static bool isCommand(const QString &str, const CommandIdType cmd)
{
    switch (cmd) {
    case CommandIdType::NORTH:
    case CommandIdType::SOUTH:
    case CommandIdType::EAST:
    case CommandIdType::WEST:
    case CommandIdType::UP:
    case CommandIdType::DOWN:
    case CommandIdType::FLEE:
        return isCommand(str, Abbrev{getLowercase(cmd), 1});

    case CommandIdType::SCOUT:
        return isCommand(str, Abbrev{getLowercase(cmd), 2});

    case CommandIdType::LOOK:
        return isCommand(str, Abbrev{getLowercase(cmd), 1}) || isCommand(str, Abbrev{"examine", 3});

    case CommandIdType::UNKNOWN:
    case CommandIdType::NONE:
        return false;
    }

    return false;
}

bool AbstractParser::parseUserCommands(const QString &input)
{
    if (tryParseGenericDoorCommand(input))
        return false;

    const auto &prefixChar = getConfig().parser.prefixChar;
    if (input.startsWith(prefixChar)) {
        auto view = StringView{input}.trim();
        if (view.isEmpty() || view.takeFirstLetter() != prefixChar)
            sendToUser("Internal error. Sorry.\r\n");
        else
            parseSpecialCommand(view);
        sendPromptToUser();
        return false;
    }

    return parseSimpleCommand(input);
}

bool AbstractParser::parseSimpleCommand(const QString &str)
{
    const auto isOnline = ::isOnline();

    for (const CommandIdType cmd : ALL_COMMANDS) {
        if (cmd == CommandIdType::NONE || cmd == CommandIdType::UNKNOWN)
            continue;

        if (!isCommand(str, cmd))
            continue;

        switch (cmd) {
        case CommandIdType::NORTH:
        case CommandIdType::SOUTH:
        case CommandIdType::EAST:
        case CommandIdType::WEST:
        case CommandIdType::UP:
        case CommandIdType::DOWN:
        case CommandIdType::LOOK:
            doMove(cmd);
            return isOnline;

        case CommandIdType::FLEE:
            if (!isOnline) {
                offlineCharacterMove(CommandIdType::FLEE);
                return false; // do not send command to mud server for offline mode
            }
            break;

        case CommandIdType::SCOUT:
            if (!isOnline) {
                auto view = StringView{str}.trim();
                if (!view.isEmpty() && !view.takeFirstWord().isEmpty()) {
                    const auto dir = static_cast<CommandIdType>(tryGetDir(view));
                    if (dir >= CommandIdType::UNKNOWN) {
                        sendToUser("In which direction do you want to scout?\r\n");
                        sendPromptToUser();

                    } else {
                        queue.enqueue(CommandIdType::SCOUT);
                        queue.enqueue(dir);
                        offlineCharacterMove();
                    }
                    return false;
                }
            }
            break;

        case CommandIdType::UNKNOWN:
        case CommandIdType::NONE:
            break;
        }

        break; /* break the for loop */
    }

    if (!isOnline) {
        sendToUser("Arglebargle, glop-glyf!?!\r\n");
        sendPromptToUser();
    }

    return isOnline; // only forward command to mud server if online
}

bool AbstractParser::parseDoorAction(StringView words)
{
    if (words.isEmpty())
        return false;

    const auto firstWord = words.takeFirstWord();
    for (const DoorActionType dat : ALL_DOOR_ACTION_TYPES) {
        if (getParserCommandName(dat).matches(firstWord)) {
            return parseDoorAction(dat, words);
        }
    }
    return false;
}

bool AbstractParser::parseDoorAction(const DoorActionType dat, StringView words)
{
    const auto dir = tryGetDir(words);
    if (!words.isEmpty())
        return false;
    performDoorCommand(dir, dat);
    return true;
}

bool AbstractParser::parseDoorFlags(StringView words)
{
    if (words.isEmpty())
        return false;

    const auto firstWord = words.takeFirstWord();
    for (const DoorFlag flag : ALL_DOOR_FLAGS) {
        if (getParserCommandName(flag).matches(firstWord)) {
            return parseDoorFlag(flag, words);
        }
    }
    return false;
}

bool AbstractParser::parseDoorFlag(const DoorFlag flag, StringView words)
{
    const auto dir = tryGetDir(words);
    if (!words.isEmpty())
        return false;
    toggleDoorFlagCommand(flag, dir);
    return true;
}

bool AbstractParser::parseExitFlags(StringView words)
{
    if (words.isEmpty())
        return false;

    const auto firstWord = words.takeFirstWord();
    for (const ExitFlag flag : ALL_EXIT_FLAGS) {
        if (getParserCommandName(flag).matches(firstWord)) {
            return parseExitFlag(flag, words);
        }
    }
    return false;
}

bool AbstractParser::parseExitFlag(const ExitFlag flag, StringView words)
{
    const auto dir = tryGetDir(words);
    if (!words.isEmpty())
        return false;
    toggleExitFlagCommand(flag, dir);
    return true;
}

bool AbstractParser::parseField(StringView words)
{
#define PARSE_FIELD(X) \
    for (const auto x : DEFINED_ROOM_##X##_TYPES) { \
        if (getParserCommandName(x).matches(firstWord)) { \
            setRoomFieldCommand(x, RoomField::X##_TYPE); \
            return true; \
        } \
    }

    if (words.isEmpty())
        return false;

    // REVISIT: support "set room field XXX" ?
    const auto firstWord = words.takeFirstWord();
    if (!words.isEmpty())
        return false;

    PARSE_FIELD(LIGHT);
    PARSE_FIELD(SUNDEATH);
    PARSE_FIELD(PORTABLE);
    PARSE_FIELD(RIDABLE);
    PARSE_FIELD(ALIGN);
    return false;
#undef PARSE
}

bool AbstractParser::parseMobFlags(StringView words)
{
    if (words.isEmpty())
        return false;

    const auto firstWord = words.takeFirstWord();
    if (!words.isEmpty())
        return false;

    for (const RoomMobFlag mobFlag : ALL_MOB_FLAGS) {
        if (getParserCommandName(mobFlag).matches(firstWord)) {
            toggleRoomFlagCommand(mobFlag, RoomField::MOB_FLAGS);
            return true;
        }
    }
    return false;
}
bool AbstractParser::parseLoadFlags(StringView words)
{
    if (words.isEmpty())
        return false;

    const auto firstWord = words.takeFirstWord();
    if (!words.isEmpty())
        return false;

    for (const RoomLoadFlag loadFlag : ALL_LOAD_FLAGS) {
        if (getParserCommandName(loadFlag).matches(firstWord)) {
            toggleRoomFlagCommand(loadFlag, RoomField::LOAD_FLAGS);
            return true;
        }
    }
    return false;
}

void AbstractParser::parseSetCommand(StringView view)
{
    if (view.isEmpty()) {
        sendToUser("Set what? [prefix]\r\n");
        return;
    }

    auto first = view.takeFirstWord();
    if (Abbrev{"prefix", 3}.matches(first)) {
        if (view.isEmpty()) {
            showCommandPrefix();
            return;
        }

        auto next = view.takeFirstWord();
        if (next.size() == 3) {
            auto quote = next.takeFirstLetter();
            const bool validQuote = quote == '\'' || quote == '"';
            const auto prefix = next.takeFirstLetter().toLatin1();

            if (validQuote && isValidPrefix(prefix) && quote == next.takeFirstLetter()
                && setCommandPrefix(prefix)) {
                return;
            }
        } else if (next.size() == 1) {
            const auto prefix = next.takeFirstLetter().toLatin1();
            if (setCommandPrefix(prefix)) {
                return;
            }
        }

        sendToUser("Invalid prefix.\r\n");
        return;
    }

    sendToUser("That variable is not supported.");
}

bool AbstractParser::parsePrint(StringView &input)
{
    const auto syntax = [this]() { sendToUser("Print what? [dynamic | static | note]\r\n"); };

    if (input.isEmpty()) {
        syntax();
        return true;
    }

    const auto next = input.takeFirstWord();
    if (Abbrev{"dynamic", 1}.matches(next)) {
        printRoomInfo(dynamicRoomFields);
        return true;
    } else if (Abbrev{"static", 1}.matches(next)) {
        printRoomInfo(staticRoomFields);
        return true;
    } else if (Abbrev{"note", 1}.matches(next)) {
        showNote();
        return true;
    } else {
        syntax();
        return true;
    }
}

void AbstractParser::parseName(StringView view)
{
    if (!view.isEmpty()) {
        auto dir = tryGetDir(view);
        if (!view.isEmpty()) {
            auto name = view.takeFirstWord();
            nameDoorCommand(name.toQString(), dir);
            return;
        }
    }

    showSyntax("name <dir> <name>");
}

void AbstractParser::parseSpecialCommand(StringView wholeCommand)
{
    if (wholeCommand.isEmpty())
        throw std::runtime_error("input is empty");

    if (evalSpecialCommandMap(wholeCommand))
        return;

    const auto word = wholeCommand.takeFirstWord();
    sendToUser(QString("Unrecognized command: %1\r\n").arg(word.toQString()));
}

void AbstractParser::parseGroupTell(const StringView &view)
{
    if (view.isEmpty())
        sendToUser("What do you want to tell the group?\r\n");
    else {
        emit sendGroupTellEvent(view.toQByteArray());
        sendToUser("OK.\r\n");
    }
}

void AbstractParser::parseGroupKick(const StringView &view)
{
    if (view.isEmpty())
        sendToUser("Who do you want to kick from the group?\r\n");
    else {
        // REVISIT: We should change GroupManager to be a "FrontEnd" in this
        // thread and call it directly
        emit sendGroupKickEvent(view.toQByteArray().simplified());
        sendToUser("OK.\r\n");
    }
}

void AbstractParser::parseSearch(StringView view)
{
    if (view.isEmpty())
        showSyntax("search [-(name|desc|dyncdesc|note|exits|all)] pattern");
    else
        doSearchCommand(view);
}

void AbstractParser::parseDirections(StringView view)
{
    if (view.isEmpty())
        showSyntax("dirs [-(name|desc|dyncdesc|note|exits|all)] pattern");
    else
        doGetDirectionsCommand(view);
}

void AbstractParser::parseNoteCmd(StringView view)
{
    setNote(view.toQString());
}

void AbstractParser::parseHelp(StringView words)
{
    if (words.isEmpty()) {
        showHelp();
        return;
    }

    auto next = words.takeFirstWord();

    if (Abbrev{"abbreviations", 2}.matches(next)) {
        showHelpCommands(true);
        return;
    } else if (Abbrev{"commands", 1}.matches(next)) {
        showHelpCommands(false);
        return;
    }

    auto &map = m_specialCommandMap;
    auto name = next.toQString().toStdString();
    auto it = map.find(name);
    if (it != map.end()) {
        it->second.help(name);
        return;
    }

    if (Abbrev{"map", 1}.matches(next))
        showMapHelp();
    else if (Abbrev{"door", 1}.matches(next))
        showDoorCommandHelp();
    else if (Abbrev{"group", 1}.matches(next))
        showGroupHelp();
    else if (Abbrev{"exits", 2}.matches(next))
        showExitHelp();
    else if (Abbrev{"flags", 1}.matches(next))
        showRoomSimpleFlagsHelp();
    else if (Abbrev{"mobiles", 2}.matches(next))
        showRoomMobFlagsHelp();
    else if (Abbrev{"load", 2}.matches(next))
        showRoomLoadFlagsHelp();
    else if (Abbrev{"miscellaneous", 2}.matches(next))
        showMiscHelp();
    else {
        showHelp();
    }
}

void AbstractParser::initSpecialCommandMap()
{
    auto &map = m_specialCommandMap;
    map.clear();

    auto add = [this](Abbrev abb, const ParserCallback &callback, const HelpCallback &help) {
        addSpecialCommand(abb.getCommand(), abb.getMinAbbrev(), callback, help);
    };

    const auto makeSimpleHelp = [this](const std::string &help) {
        return [this, help](const std::string &name) {
            const auto &prefixChar = getConfig().parser.prefixChar;
            sendToUser(QString("Help for %1%2:\r\n  %3\r\n\r\n")
                           .arg(prefixChar)
                           .arg(QString::fromStdString(name))
                           .arg(QString::fromStdString(help)));
        };
    };

    qInfo() << "Adding special commands to the map...";

    // help is important, so it comes first

    add(cmdHelp,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseHelp(rest);
            return true;
        },
        // TODO: create a parse tree, and show all of the help topics.
        makeSimpleHelp("Provides help."));
    add(cmdMapHelp,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->showMapHelp();
            return true;
        },
        makeSimpleHelp("Help for mapping console commands."));
    add(cmdDoorHelp,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->showDoorCommandHelp();
            return true;
        },
        makeSimpleHelp("Help for door console commands."));
    add(cmdGroupHelp,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->showGroupHelp();
            return true;
        },
        makeSimpleHelp("Help for group manager console commands."));

    // door actions
    for (const DoorActionType x : ALL_DOOR_ACTION_TYPES) {
        if (auto cmd = getParserCommandName(x))
            add(cmd,
                [this, x](const std::vector<StringView> & /*s*/, StringView rest) {
                    return parseDoorAction(x, rest);
                },
                makeSimpleHelp("Sets door action: " + std::string{cmd.getCommand()}));
    }

    for (const DoorFlag x : ALL_DOOR_FLAGS) {
        if (auto cmd = getParserCommandName(x))
            add(getParserCommandName(x),
                [this, x](const std::vector<StringView> & /*s*/, StringView rest) {
                    return parseDoorFlag(x, rest);
                },
                makeSimpleHelp("Sets door flag: " + std::string{cmd.getCommand()}));
    }

    for (const ExitFlag x : ALL_EXIT_FLAGS) {
        if (auto cmd = getParserCommandName(x))
            add(cmd,
                [this, x](const std::vector<StringView> & /*s*/, StringView rest) {
                    return parseExitFlag(x, rest);
                },
                makeSimpleHelp("Sets exit flag: " + std::string{cmd.getCommand()}));
    }

#define ADD_FIELD_CMDS(X) \
    do { \
        for (const auto x : DEFINED_ROOM_##X##_TYPES) { \
            if (auto cmd = getParserCommandName(x)) { \
                auto type = RoomField::X##_TYPE; \
                add(cmd, \
                    [this, x, type](const std::vector<StringView> & /*s*/, StringView rest) { \
                        if (!rest.isEmpty()) \
                            return false; \
                        setRoomFieldCommand(x, type); \
                        return true; \
                    }, \
                    makeSimpleHelp("Sets " #X " flag: " + std::string{cmd.getCommand()})); \
            } \
        } \
    } while (false)
    ADD_FIELD_CMDS(LIGHT);
    ADD_FIELD_CMDS(SUNDEATH);
    ADD_FIELD_CMDS(PORTABLE);
    ADD_FIELD_CMDS(RIDABLE);
    ADD_FIELD_CMDS(ALIGN);
#undef ADD_FIELD_CMDS

    for (const RoomMobFlag x : ALL_MOB_FLAGS) {
        if (auto cmd = getParserCommandName(x)) {
            add(cmd,
                [this, x](const std::vector<StringView> & /*s*/, StringView rest) {
                    if (!rest.isEmpty())
                        return false;
                    toggleRoomFlagCommand(x, RoomField::MOB_FLAGS);
                    return true;
                },
                makeSimpleHelp("Sets room mob flag: " + std::string{cmd.getCommand()}));
        }
    }

    for (const RoomLoadFlag x : ALL_LOAD_FLAGS) {
        if (auto cmd = getParserCommandName(x))
            add(cmd,
                [this, x](const std::vector<StringView> & /*s*/, StringView rest) {
                    if (!rest.isEmpty())
                        return false;
                    toggleRoomFlagCommand(x, RoomField::LOAD_FLAGS);
                    return true;
                },
                makeSimpleHelp("Sets room load flag: " + std::string{cmd.getCommand()}));
    }

    // misc commands

    add(cmdBack,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->doBackCommand();
            return true;
        },
        makeSimpleHelp("Delete prespammed commands from queue."));
    add(cmdDirections,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseDirections(rest);
            return true;
        },
        makeSimpleHelp("Prints directions to matching rooms."));
    add(cmdGroupKick,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseGroupKick(rest);
            return true;
        },
        makeSimpleHelp("Kick [player] from the group."));
    add(cmdGroupLock,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->doGroupLockCommand();
            return true;
        },
        makeSimpleHelp("Toggle the lock on the group."));
    add(cmdGroupTell,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseGroupTell(rest);
            return true;
        },
        makeSimpleHelp("Send a grouptell with the [message]."));
    add(cmdMarkCurrent,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->doMarkCurrentCommand();
            return true;
        },
        makeSimpleHelp("Highlight the room you are currently in."));
    add(cmdName,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseName(rest);
            return true;
        },
        makeSimpleHelp(
            "Arguments: <dir> <name>;  Sets the name of door in direction <dir> with <name>."));
    add(cmdNote,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseNoteCmd(rest);
            return true;
        },
        makeSimpleHelp("Sets the note for the current room."));
    add(cmdRemoveDoorNames,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->doRemoveDoorNamesCommand();
            return true;
        },
        makeSimpleHelp(
            "Removes all secret door names from the current map (WARNING: destructive)!"));
    add(cmdSearch,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseSearch(rest);
            return true;
        },
        makeSimpleHelp("Highlight matching rooms on the map."));
    add(cmdSet,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseSetCommand(rest);
            return true;
        },
        makeSimpleHelp("Subcommand: prefix <punct char>; Lets you change the command prefix!"));
    add(cmdTime,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->showMumeTime();
            return true;
        },
        makeSimpleHelp("Displays the current MUME time."));
    add(cmdTrollExit,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->toggleTrollMapping();
            return true;
        },
        makeSimpleHelp("Toggles troll-only exit mapping for direct sunlight."));
    add(cmdVote,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->openVoteURL();
            return true;
        },
        makeSimpleHelp("Launches a web browser so you can vote for MUME on TMC!"));

    /* print commands */
    add(cmdPrint,
        [this](const std::vector<StringView> & /*s*/, StringView rest) { return parsePrint(rest); },
        makeSimpleHelp("There is no help for this command yet."));

    add(cmdPDynamic,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            printRoomInfo(dynamicRoomFields);
            return true;
        },
        makeSimpleHelp("Prints current room description."));
    add(cmdPStatic,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            printRoomInfo(staticRoomFields);
            return true;
        },
        makeSimpleHelp("Prints current room description without movable items."));

    add(cmdPNote,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            showNote();
            return true;
        },
        makeSimpleHelp("Print the note in the current room."));

    qInfo() << "Total commands + abbreviations: " << map.size();
}

void AbstractParser::addSpecialCommand(const char *const s,
                                       const int minLen,
                                       const ParserCallback &callback,
                                       const HelpCallback &help)
{
    const auto abb = Abbrev{s, minLen};
    if (!abb)
        throw std::invalid_argument("s");

    const auto len = abb.getLength();
    const auto min = std::max(1, abb.getMinAbbrev());
    const std::string fullName = abb.getCommand();
    std::string key = fullName;
    auto &map = m_specialCommandMap;
    for (int i = len; i >= min; --i) {
        assert(i >= 0);
        key.resize(static_cast<unsigned int>(i));
        auto it = map.find(key);
        if (it == map.end())
            map.emplace(key, ParserRecord{fullName, callback, help});
        else {
            qWarning() << ("unable to add " + QString::fromStdString(key) + " for "
                           + abb.describe());
        }
    }
}

bool AbstractParser::evalSpecialCommandMap(StringView args)
{
    if (args.empty())
        return false;

    auto first = args.takeFirstWord();
    auto &map = m_specialCommandMap;

    const std::string key = first.toQString().toStdString();
    auto it = map.find(key);
    if (it == map.end())
        return false;

    // REVISIT: add # of calls to the record?
    ParserRecord &rec = it->second;
    const auto qs = QString::fromStdString(rec.fullCommand);
    const auto matched = std::vector<StringView>{StringView{qs}};
    return rec.callback(matched, args);
}
