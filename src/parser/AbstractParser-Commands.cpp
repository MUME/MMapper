// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractParser-Commands.h"

#include <algorithm>
#include <functional>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <QMessageLogContext>
#include <QtCore>

#include "../global/StringView.h"
#include "../global/TextUtils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/enums.h"
#include "../mapdata/mmapper2room.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "Abbrev.h"
#include "AbstractParser-Utils.h"
#include "CommandId.h"
#include "DoorAction.h"
#include "abstractparser.h"

const Abbrev cmdBack{"back"};
const Abbrev cmdConfig{"config", 4};
const Abbrev cmdConnect{"connect", 4};
const Abbrev cmdDirections{"dirs", 3};
const Abbrev cmdDisconnect{"disconnect", 4};
const Abbrev cmdDoorHelp{"doorhelp", 5};
const Abbrev cmdGroup{"group", 5};
const Abbrev cmdGroupTell{"gtell", 2};
const Abbrev cmdHelp{"help", 2};
const Abbrev cmdMarkCurrent{"markcurrent", 4};
const Abbrev cmdRemoveDoorNames{"removedoornames"};
const Abbrev cmdRoom{"room", 2};
const Abbrev cmdSearch{"search", 3};
const Abbrev cmdSet{"set", 2};
const Abbrev cmdTime{"time", 2};
const Abbrev cmdTrollExit{"trollexit", 2};
const Abbrev cmdVote{"vote", 2};

Abbrev getParserCommandName(const DoorFlagEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case DoorFlagEnum::UPPER: \
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
        CASE3(KNOCKABLE, "knockable", 6);
        CASE3(MAGIC, "magic", 3);
        CASE3(ACTION, "action", 3);
        CASE3(NO_BASH, "nobash", -1);
    }
    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const RoomLightEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomLightEnum::UPPER: \
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

Abbrev getParserCommandName(const RoomSundeathEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomSundeathEnum::UPPER: \
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

Abbrev getParserCommandName(const RoomPortableEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomPortableEnum::UPPER: \
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

Abbrev getParserCommandName(const RoomRidableEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomRidableEnum::UPPER: \
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

Abbrev getParserCommandName(const RoomAlignEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomAlignEnum::UPPER: \
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

Abbrev getParserCommandName(const RoomMobFlagEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomMobFlagEnum::UPPER: \
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

Abbrev getParserCommandName(const RoomLoadFlagEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomLoadFlagEnum::UPPER: \
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
        CASE3(COACH, "coach", -1);
        CASE3(FERRY, "ferry", -1);
    }
    return Abbrev{};
#undef CASE3
}

// NOTE: This isn't used by the parser (currently only used for filenames).
Abbrev getParserCommandName(const RoomTerrainEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case RoomTerrainEnum::UPPER: \
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

QByteArray getCommandName(const DoorActionEnum action)
{
#define CASE2(UPPER, s) \
    do { \
    case DoorActionEnum::UPPER: \
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
        CASE2(KNOCK, "knock");

    case DoorActionEnum::NONE:
        break;
    }

    // REVISIT: use "look" ?
    return "";
#undef CASE2
}

Abbrev getParserCommandName(const DoorActionEnum action)
{
#define CASE3(UPPER, s, n) \
    do { \
    case DoorActionEnum::UPPER: \
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
        CASE3(KNOCK, "knock", -1);
    case DoorActionEnum::NONE:
        break;
    }

    return Abbrev{};
#undef CASE3
}

Abbrev getParserCommandName(const ExitFlagEnum x)
{
#define CASE3(UPPER, s, n) \
    do { \
    case ExitFlagEnum::UPPER: \
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

static bool isCommand(const std::string &str, Abbrev abbrev)
{
    if (!abbrev)
        return false;

    auto view = StringView{str}.trim();
    if (view.isEmpty())
        return false;

    const auto word = view.takeFirstWord();
    return abbrev.matches(word);
}

static bool isCommand(const std::string &str, const CommandEnum cmd)
{
    switch (cmd) {
    case CommandEnum::NORTH:
    case CommandEnum::SOUTH:
    case CommandEnum::EAST:
    case CommandEnum::WEST:
    case CommandEnum::UP:
    case CommandEnum::DOWN:
    case CommandEnum::FLEE:
        // REVISIT: Add support for 'charge' and 'escape' commands
        return isCommand(str, Abbrev{getLowercase(cmd), 1});

    case CommandEnum::SCOUT:
        return isCommand(str, Abbrev{getLowercase(cmd), 2});

    case CommandEnum::LOOK:
        return isCommand(str, Abbrev{getLowercase(cmd), 1}) || isCommand(str, Abbrev{"examine", 3});

    case CommandEnum::UNKNOWN:
    case CommandEnum::NONE:
        return false;
    }

    return false;
}

bool AbstractParser::parseUserCommands(const QString &input)
{
    if (tryParseGenericDoorCommand(input))
        return false;

    if (input.startsWith(prefixChar)) {
        std::string s = ::toStdStringLatin1(input);
        auto view = StringView{s}.trim();
        if (view.isEmpty() || view.takeFirstLetter() != prefixChar)
            sendToUser("Internal error. Sorry.\r\n");
        else
            parseSpecialCommand(view);
        sendPromptToUser();
        return false;
    }

    return parseSimpleCommand(input);
}

bool AbstractParser::parseSimpleCommand(const QString &qstr)
{
    const std::string str = ::toStdStringLatin1(qstr);
    const auto isOnline = ::isOnline();

    for (const CommandEnum cmd : ALL_COMMANDS) {
        if (cmd == CommandEnum::NONE || cmd == CommandEnum::UNKNOWN)
            continue;

        if (!isCommand(str, cmd))
            continue;

        switch (cmd) {
        case CommandEnum::NORTH:
        case CommandEnum::SOUTH:
        case CommandEnum::EAST:
        case CommandEnum::WEST:
        case CommandEnum::UP:
        case CommandEnum::DOWN:
            doMove(cmd);
            return isOnline;

        case CommandEnum::LOOK: {
            // if 'look' has arguments then it isn't valid for prespam
            auto view = StringView{str}.trim();
            if (view.countWords() == 1) {
                doMove(cmd);
                return isOnline;
            }
        } break;

        case CommandEnum::FLEE:
            if (!isOnline) {
                offlineCharacterMove(CommandEnum::FLEE);
                return false; // do not send command to mud server for offline mode
            }
            break;

        case CommandEnum::SCOUT:
            if (!isOnline) {
                auto view = StringView{str}.trim();
                if (!view.isEmpty() && !view.takeFirstWord().isEmpty()) {
                    const auto dir = static_cast<CommandEnum>(tryGetDir(view));
                    if (dir >= CommandEnum::UNKNOWN) {
                        sendToUser("In which direction do you want to scout?\r\n");
                        sendPromptToUser();

                    } else {
                        m_queue.enqueue(CommandEnum::SCOUT);
                        m_queue.enqueue(dir);
                        offlineCharacterMove();
                    }
                    return false;
                }
            }
            break;

        case CommandEnum::UNKNOWN:
        case CommandEnum::NONE:
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
    for (const DoorActionEnum dat : ALL_DOOR_ACTION_TYPES) {
        if (getParserCommandName(dat).matches(firstWord)) {
            return parseDoorAction(dat, words);
        }
    }
    return false;
}

bool AbstractParser::parseDoorAction(const DoorActionEnum dat, StringView words)
{
    const auto dir = tryGetDir(words);
    if (!words.isEmpty())
        return false;
    performDoorCommand(dir, dat);
    return true;
}

void AbstractParser::parseSetCommand(StringView view)
{
    if (view.isEmpty()) {
        sendToUser(QString("Syntax: %1set prefix [punct-char]\r\n").arg(prefixChar));
        return;
    }

    auto first = view.takeFirstWord();
    if (Abbrev{"prefix", 3}.matches(first)) {
        if (view.isEmpty()) {
            showCommandPrefix();
            return;
        }

        auto next = view.takeFirstWord();
        if (next.length() == 3) {
            auto quote = next.takeFirstLetter();
            const bool validQuote = quote == '\'' || quote == '"';
            const auto prefix = next.takeFirstLetter();

            if (validQuote && isValidPrefix(prefix) && quote == next.takeFirstLetter()
                && quote != prefix && setCommandPrefix(prefix)) {
                return;
            }
        } else if (next.length() == 1) {
            const auto prefix = next.takeFirstLetter();
            if (setCommandPrefix(prefix)) {
                return;
            }
        }

        sendToUser("Invalid prefix.\r\n");
        return;
    }

    sendToUser("That variable is not supported.");
}

void AbstractParser::parseSpecialCommand(StringView wholeCommand)
{
    // TODO: Just use '\n' and transform to "\r\n" elsewhere if necessary.
    if (wholeCommand.isEmpty()) {
        sendToUser("Error: special command input is empty.\r\n");
        return;
    }

    if (evalSpecialCommandMap(wholeCommand))
        return;

    const auto word = wholeCommand.takeFirstWord();
    sendToUser(QString("Unrecognized command: %1\r\n").arg(word.toQString()));
}

void AbstractParser::parseSearch(StringView view)
{
    if (view.isEmpty())
        showSyntax("search [-(name|desc|dyncdesc|note|exits|flags|all|clear)] pattern");
    else
        doSearchCommand(view);
}

void AbstractParser::parseDirections(StringView view)
{
    if (view.isEmpty())
        showSyntax("dirs [-(name|desc|dyncdesc|note|exits|flags|all)] pattern");
    else
        doGetDirectionsCommand(view);
}

class ArgHelpCommand final : public syntax::IArgument
{
private:
    const AbstractParser::ParserRecordMap &m_map;

public:
    explicit ArgHelpCommand(const AbstractParser::ParserRecordMap &map)
        : m_map(map)
    {}

private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger * /*logger*/) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgHelpCommand::virt_match(const syntax::ParserInput &input,
                                               syntax::IMatchErrorLogger *) const
{
    auto &arg = *this;
    if (!input.empty()) {
        const auto &map = arg.m_map;
        const auto &next = input.front();
        const auto it = map.find(next);
        if (it != map.end()) {
            return syntax::MatchResult::success(1, input, Value(next));
        }
    }

    return syntax::MatchResult::failure(input);
}

std::ostream &ArgHelpCommand::virt_to_stream(std::ostream &os) const
{
    return os << "<command>";
}

void AbstractParser::parseHelp(StringView words)
{
    using namespace syntax;
    static auto abb = syntax::abbrevToken;
    auto simpleSyntax = [](const std::string_view &name, auto &&fn) -> SharedConstSublist {
        auto abbrev = abb(std::string(name));
        auto helpstr = std::string("help for ") + std::string(name);
        return buildSyntax(std::move(abbrev), Accept(fn, std::move(helpstr)));
    };

    auto syntax = buildSyntax(
        /* Note: The next section has optional "topic" to avoid collision. */
        buildSyntax(TokenMatcher::alloc<ArgHelpCommand>(m_specialCommandMap),
                    Accept(
                        [this](User &user, const Pair *const matched) -> void {
                            auto &os = user.getOstream();
                            if (!matched || !matched->car.isString()) {
                                os << "Internal error.\n";
                                return;
                            }
                            const auto &map = m_specialCommandMap;
                            const auto &name = matched->car.getString();
                            const auto it = map.find(name);
                            if (it == map.end()) {
                                os << "Internal error.\n";
                                return;
                            }
                            it->second.help(name);
                        },
                        "detailed help pages")),

        /* optional "topic" allows you to [/help topic door] to see the door help topic;
         * alternately, "[/help doors] should work too. */
        buildSyntax(TokenMatcher::alloc<ArgOptionalToken>(abb("topic")),
                    simpleSyntax("abbreviations",
                                 [this](auto &&, auto &&) { showHelpCommands(true); }),
                    simpleSyntax("commands", [this](auto &&, auto &&) { showHelpCommands(false); }),
                    simpleSyntax("doors", [this](auto &&, auto &&) { showDoorCommandHelp(); }),
                    simpleSyntax("miscellaneous", [this](auto &&, auto &&) { showMiscHelp(); })),
        Accept([this](auto &&, auto &&) { showHelp(); }, "general help"));

    eval("help", syntax, words);
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
            sendToUser(QString("Help for %1%2:\r\n  %3\r\n\r\n")
                           .arg(prefixChar)
                           .arg(::toQStringLatin1(name))
                           .arg(::toQStringLatin1(help)));
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
    add(cmdDoorHelp,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            if (!rest.isEmpty())
                return false;
            this->showDoorCommandHelp();
            return true;
        },
        makeSimpleHelp("Help for door console commands."));

    // door actions
    for (const DoorActionEnum x : ALL_DOOR_ACTION_TYPES) {
        if (auto cmd = getParserCommandName(x))
            add(cmd,
                [this, x](const std::vector<StringView> & /*s*/, StringView rest) {
                    return parseDoorAction(x, rest);
                },
                makeSimpleHelp("Sets door action: " + std::string{cmd.getCommand()}));
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
    add(cmdConfig,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->doConfig(rest);
            return true;
        },
        makeSimpleHelp("Configuration commands."));
    add(cmdConnect,
        [this](const std::vector<StringView> & /*s*/, StringView /*rest*/) {
            this->doConnectToHost();
            return true;
        },
        makeSimpleHelp("Connect to the MUD."));
    add(cmdDirections,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            this->parseDirections(rest);
            return true;
        },
        makeSimpleHelp("Prints directions to matching rooms."));
    add(cmdDisconnect,
        [this](const std::vector<StringView> & /*s*/, StringView /*rest*/) {
            this->doDisconnectFromHost();
            return true;
        },
        makeSimpleHelp("Disconnect from the MUD."));
    add(cmdGroupTell,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            const auto alias = "tell " + rest.toStdString();
            parseGroup(StringView{alias});
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
        [this](const std::string &name) {
            const char help[]
                = "Subcommands:\r\n"
                  "\tprefix              # Displays the current prefix.\r\n"
                  "\tprefix <punct-char> # Changes the current prefix.\r\n"
                  "\r\n"
                  // REVISIT: Does it actually support LATIN-1 punctuation like the degree symbol?
                  "Note: <punct-char> may be any ASCII punctuation character,\r\n"
                  "      which can be optionally single- or double-quoted.\r\n"
                  "\r\n"
                  "Examples to set prefix:\r\n"
                  "\tprefix /   # slash character\r\n"
                  "\tprefix '/' # single-quoted slash character\r\n"
                  "\tprefix \"/\" # double-quoted slash character\r\n"
                  "\tprefix '   # bare single-quote character\r\n"
                  "\tprefix \"'\" # double-quoted single-quote character\r\n"
                  "\tprefix \"   # bare double-quote character\r\n"
                  "\tprefix '\"' # single-quoted double-quote character\r\n"
                  "\r\n"
                  "Note: Quoted versions do not allow escape codes,\r\n"
                  "so you cannot do ''', '\\'', \"\"\", or \"\\\"\".";
            sendToUser(QString("Help for %1%2:\r\n%3\r\n\r\n")
                           .arg(prefixChar)
                           .arg(::toQStringLatin1(name))
                           .arg(::toQStringLatin1(help)));
        });
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

    /* room commands */
    add(cmdRoom,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            parseRoom(rest);
            return true;
        },
        makeSimpleHelp("View or modify properties of the current room."));

    /* group commands */
    add(cmdGroup,
        [this](const std::vector<StringView> & /*s*/, StringView rest) {
            parseGroup(rest);
            return true;
        },
        makeSimpleHelp("Perform actions on the group manager."));

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
            qWarning() << ("unable to add " + ::toQStringLatin1(key) + " for " + abb.describe());
        }
    }
}

bool AbstractParser::evalSpecialCommandMap(StringView args)
{
    if (args.empty())
        return false;

    auto first = args.takeFirstWord();
    auto &map = m_specialCommandMap;

    const std::string key = toLowerLatin1(first.getStdStringView());
    auto it = map.find(key);
    if (it == map.end())
        return false;

    // REVISIT: add # of calls to the record?
    ParserRecord &rec = it->second;
    const auto &s = rec.fullCommand;
    const auto matched = std::vector<StringView>{StringView{s}};
    return rec.callback(matched, args);
}
