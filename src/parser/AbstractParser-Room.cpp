// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "abstractparser.h"

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

#include "../configuration/configuration.h"
#include "../expandoracommon/room.h"
#include "../global/TextUtils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/customaction.h"
#include "../mapdata/enums.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Commands.h"
#include "AbstractParser-Utils.h"

static constexpr const auto staticRoomFields = RoomFieldEnum::NAME | RoomFieldEnum::DESC;
static constexpr const auto dynamicRoomFields = staticRoomFields | RoomFieldEnum::CONTENTS;

NODISCARD static const char *getFlagName(const ExitFlagEnum flag)
{
#define CASE(UPPER, s) \
    do { \
    case ExitFlagEnum::UPPER: \
        return s; \
    } while (false)
    switch (flag) {
        CASE(EXIT, "Possible");
        CASE(DOOR, "Door");
        CASE(ROAD, "Road");
        CASE(CLIMB, "Climbable");
        CASE(RANDOM, "Random");
        CASE(SPECIAL, "Special");
        CASE(NO_MATCH, "No match");
        CASE(FLOW, "Water flow");
        CASE(NO_FLEE, "No flee");
        CASE(DAMAGE, "Damage");
        CASE(FALL, "Fall");
        CASE(GUARDED, "Guarded");
    }
    return "Unknown";
#undef CASE
}

NODISCARD static const char *getFlagName(const DoorFlagEnum flag)
{
#define CASE(UPPER, s) \
    do { \
    case DoorFlagEnum::UPPER: \
        return s; \
    } while (false)
    switch (flag) {
        CASE(HIDDEN, "Hidden");
        CASE(NEED_KEY, "Need key");
        CASE(NO_BLOCK, "No block");
        CASE(NO_BREAK, "No break");
        CASE(NO_PICK, "No pick");
        CASE(DELAYED, "Delayed");
        CASE(CALLABLE, "Callable");
        CASE(KNOCKABLE, "Knockable");
        CASE(MAGIC, "Magic");
        CASE(ACTION, "Action");
        CASE(NO_BASH, "No bash");
    }
    return "Unknown";
#undef CASE
}

NODISCARD static std::optional<ExitDirEnum> findLowercaseDirAbbrev(const std::string_view input)
{
    if (input.empty()) {
        return std::nullopt;
    }

    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        if (isAbbrev(input, lowercaseDirection(dir))) {
            return dir;
        }
    }

    return std::nullopt;
}

NODISCARD static std::optional<RoomFieldVariant> evalRoomField(const std::string_view args)
{
    using ParserRoomFieldMap = std::map<std::string, RoomFieldVariant>;
    static const auto map = []() -> ParserRoomFieldMap {
        ParserRoomFieldMap result;

        auto add = [&result](auto &&flags, auto &&convert) {
            for (const auto flag : flags) {
                static_assert(std::is_enum_v<decltype(flag)>);
                const auto abb = getParserCommandName(flag);
                if (!abb)
                    throw std::invalid_argument("flag");
                const std::string key = abb.getCommand();
                const auto it = result.find(key);
                if (it == result.end())
                    result.emplace(key, convert(flag));
                else {
                    qWarning() << ("unable to add " + ::toQStringLatin1(key) + " for "
                                   + abb.describe());
                }
            }
        };
        auto identity = [](auto flag) { return flag; };

        // REVISIT: separate these into their own args, and don't try to group them.
        // (Hint: That would allow you set each category as "UNDEFINED.")

        add(ALL_MOB_FLAGS, [](auto flag) { return RoomMobFlags{flag}; });
        add(ALL_LOAD_FLAGS, [](auto flag) { return RoomLoadFlags{flag}; });
        add(DEFINED_ROOM_ALIGN_TYPES, identity);
        add(DEFINED_ROOM_LIGHT_TYPES, identity);
        add(DEFINED_ROOM_RIDABLE_TYPES, identity);
        add(DEFINED_ROOM_PORTABLE_TYPES, identity);
        add(DEFINED_ROOM_SUNDEATH_TYPES, identity);
        return result;
    }();
    const std::string key = toLowerLatin1(args);
    const auto it = map.find(key);
    if (it == map.end())
        return std::nullopt;
    return it->second;
}

class NODISCARD ArgDirection final : public syntax::IArgument
{
private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgDirection::virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *const logger) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    const auto arg = toLowerLatin1(input.front());

    if (const auto optDir = findLowercaseDirAbbrev(arg)) {
        const ExitDirEnum &dir = optDir.value();
        const auto s = lowercaseDirection(dir);
        const char c = s[0];
        return syntax::MatchResult::success(1, input, Value{c});
    }

    if (logger) {
        logger->logError("input was not a valid direction");
    }
    return syntax::MatchResult::failure(input);
}

std::ostream &ArgDirection::virt_to_stream(std::ostream &os) const
{
    return os << "<direction>";
}

NODISCARD static bool isAddFlag(StringView &sv)
{
    switch (sv.firstChar()) {
    case '-':
        sv.takeFirstLetter();
        return false;
    case '+':
        sv.takeFirstLetter();
        return true;
    default:
        return true;
    }
}

class NODISCARD ArgDoorFlag final : public syntax::IArgument
{
private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgDoorFlag::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *logger) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    const auto arg = toLowerLatin1(input.front());
    StringView sv(arg);

    std::vector<Value> values;
    values.emplace_back(isAddFlag(sv));

    for (const auto &flag : ALL_DOOR_FLAGS) {
        const auto &command = getParserCommandName(flag);
        if (!command.matches(sv))
            continue;
        values.emplace_back(flag);
        return syntax::MatchResult::success(1, input, Value(Vector(std::move(values))));
    }

    if (logger) {
        std::ostringstream os;
        for (const auto &flag : ALL_DOOR_FLAGS)
            os << getParserCommandName(flag).getCommand() << " ";
        logger->logError("input was not a valid door flag: " + os.str());
    }
    return syntax::MatchResult::failure(input);
}

std::ostream &ArgDoorFlag::virt_to_stream(std::ostream &os) const
{
    return os << "(+|-)<doorflag>";
}

class NODISCARD ArgExitFlag final : public syntax::IArgument
{
private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgExitFlag::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *logger) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    const auto arg = toLowerLatin1(input.front());
    StringView sv(arg);

    std::vector<Value> values;
    values.emplace_back(isAddFlag(sv));

    for (const auto &flag : ALL_EXIT_FLAGS) {
        const auto &command = getParserCommandName(flag);
        if (!command.matches(sv))
            continue;
        values.emplace_back(flag);
        return syntax::MatchResult::success(1, input, Value(Vector(std::move(values))));
    }

    if (logger) {
        std::ostringstream os;
        for (const auto &flag : ALL_EXIT_FLAGS)
            os << getParserCommandName(flag).getCommand() << " ";
        logger->logError("input was not a valid exit flag: " + os.str());
    }
    return syntax::MatchResult::failure(input);
}

std::ostream &ArgExitFlag::virt_to_stream(std::ostream &os) const
{
    return os << "(+|-)<exitflag>";
}

class NODISCARD ArgRoomFlag final : public syntax::IArgument
{
private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

// REVISIT: DEFINED_ROOM_* includes multiple flags but should be one: i.e. "noride" and "ride"
#define X_FOREACH_ARG_ROOM_FLAG(X) \
    X(ALL_MOB_FLAGS) \
    X(ALL_LOAD_FLAGS) \
    X(DEFINED_ROOM_ALIGN_TYPES) \
    X(DEFINED_ROOM_LIGHT_TYPES) \
    X(DEFINED_ROOM_RIDABLE_TYPES) \
    X(DEFINED_ROOM_PORTABLE_TYPES) \
    X(DEFINED_ROOM_SUNDEATH_TYPES)
syntax::MatchResult ArgRoomFlag::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *logger) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    const auto arg = toLowerLatin1(input.front());
    StringView sv(arg);

    std::vector<Value> values;
    values.emplace_back(isAddFlag(sv));

#define DEFINE_MATCH_LOGIC(FLAGS) \
    for (const auto &flag : FLAGS) { \
        const auto &command = getParserCommandName(flag); \
        if (!command.matches(sv)) \
            continue; \
        values.emplace_back(std::string{command.getCommand()}); \
        return syntax::MatchResult::success(1, input, Value(Vector(std::move(values)))); \
    }
    X_FOREACH_ARG_ROOM_FLAG(DEFINE_MATCH_LOGIC)
#undef DEFINE_MATCH_LOGIC

    if (logger) {
        std::ostringstream os;
#define DEFINE_VALID_FLAGS_LOGIC(FLAGS) \
    for (const auto &flag : FLAGS) \
        os << getParserCommandName(flag).getCommand() << " ";
        X_FOREACH_ARG_ROOM_FLAG(DEFINE_VALID_FLAGS_LOGIC)
#undef DEFINE_VALID_FLAGS_LOGIC
        logger->logError("input was not a valid room flag: " + os.str());
    }
    return syntax::MatchResult::failure(input);
}
#undef X_FOREACH_ARG_ROOM_FLAG

std::ostream &ArgRoomFlag::virt_to_stream(std::ostream &os) const
{
    return os << "(+|-)<roomflag>";
}

void AbstractParser::parseRoom(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    auto printDynamic = Accept([this](User &,
                                      const Pair *) -> void { printRoomInfo(dynamicRoomFields); },
                               "print room dynamic");
    auto printStatic = Accept([this](User &,
                                     const Pair *) -> void { printRoomInfo(staticRoomFields); },
                              "print room static");
    auto printNote = Accept([this](User &, const Pair *) -> void { showNote(); }, "print room note");

    auto printSyntax = buildSyntax(abb("print"),
                                   buildSyntax(abb("dynamic"), printDynamic),
                                   buildSyntax(abb("static"), printStatic),
                                   buildSyntax(abb("note"), printNote));

    auto hasDoor = [this](const ExitDirEnum dir) {
        const Coordinate c = getTailPosition();
        RoomSelection rs(m_mapData);
        if (const auto &r = rs.getRoom(c)) {
            return r->exit(dir).isDoor();
        }
        return false;
    };

    auto modifyDoorFlag = Accept(
        [this, hasDoor](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const ExitDirEnum &dir = Mmapper2Exit::dirForChar(v[1].getChar());
            if (!hasDoor(dir))
                throw std::runtime_error("exit is missing exitflag 'door'");

            const Vector &parts = v[2].getVector();
            for (const auto &part : parts) {
                const Vector &vector = part.getVector();

                const FlagModifyModeEnum mode = vector[0].getBool() ? FlagModifyModeEnum::SET
                                                                    : FlagModifyModeEnum::UNSET;

                const DoorFlagEnum flag = vector[1].getDoorFlag();
                ExitFieldVariant variant = ExitFieldVariant{DoorFlags{flag}};

                const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
                const RoomId roomId = [&rs]() {
                    if (rs->size() != 1)
                        throw std::runtime_error("unable to select current room");
                    return rs->getFirstRoomId();
                }();

                if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                           std::make_unique<ModifyExitFlags>(variant, dir, mode),
                                           roomId),
                                       rs))
                    throw std::runtime_error("execute failed");
                const auto toggle = enabledString(mode == FlagModifyModeEnum::SET);
                os << "--->" << getFlagName(flag) << " door " << toggle << std::endl;
            }
            send_ok(os);
        },
        "modify door flag");

    auto setDoorName = Accept(
        [this, hasDoor](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const ExitDirEnum &dir = Mmapper2Exit::dirForChar(v[1].getChar());
            if (!hasDoor(dir))
                throw std::runtime_error("exit is missing exitflag 'door'");

            const DoorName name(v[4].getString());

            const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
            const RoomId roomId = [&rs]() {
                if (rs->size() != 1)
                    throw std::runtime_error("unable to select current room");
                return rs->getFirstRoomId();
            }();

            if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                       std::make_unique<ModifyExitFlags>(ExitFieldVariant{name},
                                                                         dir,
                                                                         FlagModifyModeEnum::SET),
                                       roomId),
                                   rs))
                throw std::runtime_error("execute failed");
            send_ok(os);
        },
        "modify door name");

    auto clearDoorName = Accept(
        [this, hasDoor](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const ExitDirEnum dir = Mmapper2Exit::dirForChar(v[1].getChar());
            if (!hasDoor(dir))
                throw std::runtime_error("exit is missing exitflag 'door'");

            const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
            const RoomId roomId = [&rs]() {
                if (rs->size() != 1)
                    throw std::runtime_error("unable to select current room");
                return rs->getFirstRoomId();
            }();

            if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                       std::make_unique<ModifyExitFlags>(ExitFieldVariant{DoorName{}},
                                                                         dir,
                                                                         FlagModifyModeEnum::SET),
                                       roomId),
                                   rs))
                throw std::runtime_error("execute failed");
            send_ok(os);
        },
        "clear door name");

    auto doorSyntax = buildSyntax(abb("door"),
                                  TokenMatcher::alloc<ArgDirection>(),
                                  buildSyntax(TokenMatcher::alloc<ArgOneOrMoreToken>(
                                                  TokenMatcher::alloc<ArgDoorFlag>()),
                                              modifyDoorFlag),
                                  buildSyntax(abb("name"),
                                              buildSyntax(abb("clear"), clearDoorName),
                                              buildSyntax(abb("set"),
                                                          TokenMatcher::alloc<ArgString>(),
                                                          setDoorName)));

    auto hasExit = [this](const ExitDirEnum dir) {
        const Coordinate c = getTailPosition();
        RoomSelection rs(m_mapData);
        if (const auto r = rs.getRoom(c)) {
            return r->exit(dir).isExit();
        }
        return false;
    };

    auto modifyExitFlag = Accept(
        [this, hasExit](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const ExitDirEnum &dir = Mmapper2Exit::dirForChar(v[1].getChar());

            const Vector &parts = v[2].getVector();
            for (const auto &part : parts) {
                const Vector &vector = part.getVector();
                const FlagModifyModeEnum mode = vector[0].getBool() ? FlagModifyModeEnum::SET
                                                                    : FlagModifyModeEnum::UNSET;

                const ExitFlagEnum flag = vector[1].getExitFlag();
                ExitFieldVariant variant = ExitFieldVariant{ExitFlags{flag}};

                if (!hasExit(dir) && flag != ExitFlagEnum::EXIT)
                    throw std::runtime_error("exit is missing");

                const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
                const RoomId roomId = [&rs]() {
                    if (rs->size() != 1)
                        throw std::runtime_error("unable to select current room");
                    return rs->getFirstRoomId();
                }();

                if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                           std::make_unique<ModifyExitFlags>(variant, dir, mode),
                                           roomId),
                                       rs))
                    throw std::runtime_error("execute failed");
                const auto toggle = enabledString(mode == FlagModifyModeEnum::SET);
                os << "--->" << getFlagName(flag) << " exit " << toggle << std::endl;
            }
            send_ok(os);
        },
        "modify exit flag");

    auto exitFlagsSyntax = buildSyntax(abb("exit-flags"),
                                       TokenMatcher::alloc<ArgDirection>(),
                                       buildSyntax(TokenMatcher::alloc<ArgOneOrMoreToken>(
                                                       TokenMatcher::alloc<ArgExitFlag>()),
                                                   modifyExitFlag));

    auto modifyRoomFlag = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            // REVISIT: Some flags conflict with each other
            const Vector &flags = v[1].getVector();
            for (const auto &flag : flags) {
                const Vector &vector = flag.getVector();
                const FlagModifyModeEnum mode = vector[0].getBool() ? FlagModifyModeEnum::SET
                                                                    : FlagModifyModeEnum::UNSET;

                const std::optional<RoomFieldVariant> opt = evalRoomField(vector[1].getString());
                if (!opt)
                    throw std::runtime_error("unable to select current room");

                const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
                const RoomId roomId = [&rs]() {
                    if (rs->size() != 1)
                        throw std::runtime_error("unable to select current room");
                    return rs->getFirstRoomId();
                }();

                if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                           std::make_unique<ModifyRoomFlags>(opt.value(), mode),
                                           roomId),
                                       rs))
                    throw std::runtime_error("execute failed");
                const auto toggle = enabledString(mode == FlagModifyModeEnum::SET);
                os << "--->Room flag " << toggle << std::endl;
            }
            send_ok(os);
        },
        "modify room flag");

    auto flagsSyntax = buildSyntax(abb("flags"),
                                   buildSyntax(TokenMatcher::alloc<ArgOneOrMoreToken>(
                                                   TokenMatcher::alloc<ArgRoomFlag>()),
                                               modifyRoomFlag));

    auto appendRoomNote = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr ((IS_DEBUG_BUILD)) {
                const auto &append = v[1].getString();
                assert(append == "append");
            }

            const std::string note = concatenate_unquoted(v[2].getVector());
            if (note.empty()) {
                os << "What do you want to append to the note?\n";
                return;
            }

            const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
            const RoomId roomId = [&rs]() {
                if (rs->size() != 1)
                    throw std::runtime_error("unable to select current room");
                return rs->getFirstRoomId();
            }();

            const auto &old = rs->getFirstRoom()->getNote();
            const RoomNote roomNote = RoomNote{old.getStdString() + "\n" + note};
            if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                       std::make_unique<ModifyRoomFlags>(RoomFieldVariant{roomNote},
                                                                         FlagModifyModeEnum::SET),
                                       roomId),
                                   rs))
                throw std::runtime_error("execute failed");
            os << "--->Note: " << roomNote.getStdString() << std::endl;
            send_ok(os);
        },
        "append room note");

    auto clearRoomNote = Accept(
        [this](User &user, const Pair * /*args*/) {
            auto &os = user.getOstream();

            const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
            const RoomId roomId = [&rs]() {
                if (rs->size() != 1)
                    throw std::runtime_error("unable to select current room");
                return rs->getFirstRoomId();
            }();

            if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                       std::make_unique<ModifyRoomFlags>(RoomFieldVariant{RoomNote{}},
                                                                         FlagModifyModeEnum::SET),
                                       roomId),
                                   rs))
                throw std::runtime_error("execute failed");
            send_ok(os);
        },
        "clear room note");

    auto setRoomNote = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[1].getString();
                assert(set == "set");
            }

            const std::string note = concatenate_unquoted(v[2].getVector());
            if (note.empty()) {
                os << "What do you want to set the note to?\n";
                return;
            }

            const auto rs = RoomSelection::createSelection(m_mapData, getTailPosition());
            const RoomId roomId = [&rs]() {
                if (rs->size() != 1)
                    throw std::runtime_error("unable to select current room");
                return rs->getFirstRoomId();
            }();

            if (!m_mapData.execute(std::make_unique<SingleRoomAction>(
                                       std::make_unique<ModifyRoomFlags>(RoomFieldVariant{RoomNote{
                                                                             note}},
                                                                         FlagModifyModeEnum::SET),
                                       roomId),
                                   rs))
                throw std::runtime_error("execute failed");
            os << "--->Note: " << note << std::endl;
            send_ok(os);
        },
        "set room note");

    auto noteSyntax
        = buildSyntax(abb("note"),
                      buildSyntax(abb("append"), TokenMatcher::alloc<ArgRest>(), appendRoomNote),
                      buildSyntax(abb("clear"), clearRoomNote),
                      buildSyntax(abb("set"), TokenMatcher::alloc<ArgRest>(), setRoomNote));

    auto selectRoom = Accept(
        [this](User &user, const Pair *const /*args*/) {
            auto &os = user.getOstream();
            const auto tmpSel = RoomSelection::createSelection(m_mapData, getTailPosition());
            emit sig_newRoomSelection(SigRoomSelection{tmpSel});
            os << "--->Current room marked temporarily on the map." << std::endl;
            send_ok(os);
        },
        "select room");

    auto selectSyntax = buildSyntax(abb("select"), selectRoom);

    auto roomSyntax = buildSyntax(doorSyntax,
                                  exitFlagsSyntax,
                                  flagsSyntax,
                                  noteSyntax,
                                  printSyntax,
                                  selectSyntax);

    eval("room", roomSyntax, input);
}
