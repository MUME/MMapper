// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/AnsiOstream.h"
#include "../global/CaseUtils.h"
#include "../global/Charset.h"
#include "../map/Diff.h"
#include "../map/RoomRevert.h"
#include "../map/enums.h"
#include "../mapdata/mapdata.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Commands.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

using exit_change_types::ModifyExitFlags;
using room_change_types::ModifyRoomFlags;

// TODO: merge with WorldBuilder's room note addition.
NODISCARD static RoomNote appendNote(const RoomNote &old, const std::string_view rest)
{
    if (rest.empty()) {
        return old;
    }

    if (old.empty()) {
        return RoomNote{std::string(rest)};
    }

    using char_consts::C_NEWLINE;
    std::string note{old.getStdStringViewUtf8()};
    if (note.back() != C_NEWLINE) {
        note += C_NEWLINE;
    }
    note += rest;
    if (note.back() != C_NEWLINE) {
        note += C_NEWLINE;
    }

    return makeRoomNote(std::move(note));
}

// REVISIT: make Optional<ArgInt> return an optional<Value>?
NODISCARD static std::optional<int> getOptionalInt(const Value &v)
{
    if (!v.isVector()) {
        return std::nullopt;
    }

    const auto &v2 = v.getVector();
    if (!v2[0].getBool()) {
        return std::nullopt;
    }

    return v2[1].getInt();
}

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

    if (input.length() >= 2 && isAbbrev(input, "unknown")) {
        return ExitDirEnum::UNKNOWN;
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
                if (!abb) {
                    throw std::invalid_argument("flag");
                }
                const std::string key = abb.getCommand();
                const auto it = result.find(key);
                if (it == result.end()) {
                    result.emplace(key, convert(flag));
                } else {
                    qWarning() << ("unable to add " + mmqt::toQStringUtf8(key) + " for "
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
    const std::string key = toLowerUtf8(args);
    const auto it = map.find(key);
    if (it == map.end()) {
        return std::nullopt;
    }
    return it->second;
}

class NODISCARD ArgDirection final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgDirection::virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *const logger) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);
    const auto arg = toLowerUtf8(input.front());

    if (const auto &optDir = findLowercaseDirAbbrev(arg)) {
        const ExitDirEnum dir = optDir.value();
        return syntax::MatchResult::success(1, input, Value{dir});
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
    case char_consts::C_MINUS_SIGN:
        std::ignore = sv.takeFirstLetter();
        return false;
    case char_consts::C_PLUS_SIGN:
        std::ignore = sv.takeFirstLetter();
        return true;
    default:
        return true;
    }
}

class NODISCARD ArgDoorFlag final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgDoorFlag::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *logger) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);
    const auto arg = toLowerUtf8(input.front());
    StringView sv(arg);

    std::vector<Value> values;
    values.emplace_back(isAddFlag(sv));

    for (const auto &flag : ALL_DOOR_FLAGS) {
        const auto &command = getParserCommandName(flag);
        if (!command.matches(sv)) {
            continue;
        }
        values.emplace_back(flag);
        return syntax::MatchResult::success(1, input, Value(Vector(std::move(values))));
    }

    if (logger) {
        std::ostringstream os;
        for (const auto &flag : ALL_DOOR_FLAGS) {
            os << getParserCommandName(flag).getCommand() << " ";
        }
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
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgExitFlag::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *logger) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);
    const auto arg = toLowerUtf8(input.front());
    StringView sv(arg);

    std::vector<Value> values;
    values.emplace_back(isAddFlag(sv));

    for (const auto &flag : ALL_EXIT_FLAGS) {
        const auto &command = getParserCommandName(flag);
        if (!command.matches(sv)) {
            continue;
        }
        values.emplace_back(flag);
        return syntax::MatchResult::success(1, input, Value(Vector(std::move(values))));
    }

    if (logger) {
        std::ostringstream os;
        for (const auto &flag : ALL_EXIT_FLAGS) {
            os << getParserCommandName(flag).getCommand() << " ";
        }
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
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

// REVISIT: DEFINED_ROOM_* includes multiple flags but should be one: i.e. "noride" and "ride"
#define XFOREACH_ARG_ROOM_FLAG(X) \
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
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);
    const auto arg = toLowerUtf8(input.front());
    StringView sv(arg);

    std::vector<Value> values;
    values.emplace_back(isAddFlag(sv));

#define X_DEFINE_MATCH_LOGIC(FLAGS) \
    for (const auto &flag : (FLAGS)) { \
        const auto &command = getParserCommandName(flag); \
        if (!command.matches(sv)) { \
            continue; \
        } \
        values.emplace_back(std::string{command.getCommand()}); \
        return syntax::MatchResult::success(1, input, Value(Vector(std::move(values)))); \
    }
    XFOREACH_ARG_ROOM_FLAG(X_DEFINE_MATCH_LOGIC)
#undef X_DEFINE_MATCH_LOGIC

    if (logger) {
        std::ostringstream os;
#define X_DEFINE_VALID_FLAGS_LOGIC(FLAGS) \
    for (const auto &flag : (FLAGS)) { \
        os << getParserCommandName(flag).getCommand() << " "; \
    }
        XFOREACH_ARG_ROOM_FLAG(X_DEFINE_VALID_FLAGS_LOGIC)
#undef X_DEFINE_VALID_FLAGS_LOGIC
        logger->logError("input was not a valid room flag: " + os.str());
    }
    return syntax::MatchResult::failure(input);
}
#undef XFOREACH_ARG_ROOM_FLAG

std::ostream &ArgRoomFlag::virt_to_stream(std::ostream &os) const
{
    return os << "(+|-)<roomflag>";
}

NODISCARD RoomId AbstractParser::getCurrentRoomId() const
{
    const auto &mapData = m_mapData;
    const auto pRoomId = mapData.getCurrentRoomId();
    if (!pRoomId) {
        throw std::runtime_error("unable to select current room");
    }

    return deref(pRoomId);
}

RoomId AbstractParser::getOtherRoom(int otherRoomId) const
{
    if (otherRoomId < 0) {
        throw std::runtime_error("RoomId cannot be negative.");
    }
    const auto otherExt = ExternalRoomId{static_cast<uint32_t>(otherRoomId)};

    auto &mapData = m_mapData;
    const auto other = mapData.findRoomHandle(otherExt);
    if (!other) {
        throw std::runtime_error("What RoomId?");
    }

    return other.getId();
}

RoomId AbstractParser::getOptionalOtherRoom(const Vector &v, const size_t index) const
{
    if (auto other = getOptionalInt(v[index])) {
        return getOtherRoom(*other);
    } else {
        return getCurrentRoomId();
    }
}

void AbstractParser::applySingleChange(const Change &change)
{
    auto &mapData = m_mapData;
    if (!mapData.applySingleChange(change)) {
        throw std::runtime_error("execute failed");
    }
}

void AbstractParser::applyChanges(const ChangeList &changes)
{
    if (changes.getChanges().empty()) {
        return;
    }

    auto &mapData = m_mapData;
    if (!mapData.applyChanges(changes)) {
        throw std::runtime_error("execute failed");
    }
}

class NODISCARD AbstractParser::ParseRoomHelper final
{
private:
    AbstractParser &m_self;

    syntax::SharedConstSublist m_syntax;
    RoomId m_roomId = INVALID_ROOMID;

public:
    explicit ParseRoomHelper(AbstractParser &self)
        : m_self{self}
        , m_syntax{createSyntax()}
    {}

private:
    NODISCARD auto &getMap() const { return m_self.m_mapData; }

private:
    NODISCARD RoomId getRoomId() const
    {
        if (m_roomId == INVALID_ROOMID) {
            throw std::runtime_error("invalid room");
        }
        return m_roomId;
    }
    NODISCARD RoomHandle getRoom() const
    {
        //
        return m_self.m_mapData.getRoomHandle(getRoomId());
    }

private:
    void printRoomInfo(User &u, const Vector & /*argv*/, const RoomFieldFlags fieldset) const
    {
        displayRoom(u.getOstream(), getRoom(), fieldset);
    }

    void onPrintDynamic(User &u, const Vector &v) const
    {
        printRoomInfo(u, v, RoomFieldEnum::NAME | RoomFieldEnum::DESC | RoomFieldEnum::CONTENTS);
    }

    void onPrintStatic(User &u, const Vector &v) const
    {
        printRoomInfo(u, v, RoomFieldEnum::NAME | RoomFieldEnum::DESC);
    }

    void onPrintNote(User &u, const Vector &v) const
    {
        if (getRoom().getNote().empty()) {
            u.getOstream() << "The room note is empty.\n";
        } else {
            printRoomInfo(u, v, RoomFieldFlags{RoomFieldEnum::NOTE});
        }
    }

    void onPreviewRoom(User &u, const Vector & /*v*/) const
    {
        previewRoom(u.getOstream(), getRoom());
    }

    void onModifyDoorFlag(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();
        const ExitDirEnum dir = v[1].getDirection();
        if (!hasDoor(dir)) {
            throw std::runtime_error("exit is missing exitflag 'door'");
        }

        const Vector &parts = v[2].getVector();
        ChangeList changeList;
        for (const auto &part : parts) {
            const Vector &vector = part.getVector();

            const FlagModifyModeEnum mode = vector[0].getBool() ? FlagModifyModeEnum::INSERT
                                                                : FlagModifyModeEnum::REMOVE;

            const DoorFlagEnum flag = vector[1].getDoorFlag();

            ExitFieldVariant variant{DoorFlags{flag}};
            changeList.add(Change{ModifyExitFlags{roomId, dir, variant, mode}});
            const auto toggle = enabledString(mode == FlagModifyModeEnum::INSERT);
            os << getFlagName(flag) << " door " << toggle << AnsiOstream::endl;
        }
        m_self.applyChanges(changeList);
        send_ok(os);
    }

    void onSetDoorName(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        const ExitDirEnum dir = v[1].getDirection();
        if (!hasDoor(dir)) {
            throw std::runtime_error("exit is missing exitflag 'door'");
        }

        // TODO: sanitize the door name
        const DoorName name(v[4].getString());

        m_self.applySingleChange(Change{
            ModifyExitFlags{roomId, dir, ExitFieldVariant{name}, FlagModifyModeEnum::ASSIGN}});

        send_ok(os);
    }

    void onClearDoorName(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        const ExitDirEnum dir = v[1].getDirection();
        if (!hasDoor(dir)) {
            throw std::runtime_error("exit is missing exitflag 'door'");
        }

        m_self.applySingleChange(Change{
            ModifyExitFlags{roomId, dir, ExitFieldVariant{DoorName{}}, FlagModifyModeEnum::CLEAR}});

        send_ok(os);
    }

    void onModifyExitFlag(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        const ExitDirEnum dir = v[1].getDirection();
        const Vector &parts = v[2].getVector();
        ChangeList changeList;
        for (const auto &part : parts) {
            const Vector &vector = part.getVector();
            const FlagModifyModeEnum mode = vector[0].getBool() ? FlagModifyModeEnum::INSERT
                                                                : FlagModifyModeEnum::REMOVE;

            const ExitFlagEnum flag = vector[1].getExitFlag();
            ExitFieldVariant variant = ExitFieldVariant{ExitFlags{flag}};

            // REVISIT: EXIT flag will be handled internally, so this test can probably be removed.
            if (true) {
                if (!hasExit(dir) && flag != ExitFlagEnum::EXIT) {
                    throw std::runtime_error("exit is missing");
                }
            }

            changeList.add(Change{ModifyExitFlags{roomId, dir, variant, mode}});

            const auto toggle = enabledString(mode == FlagModifyModeEnum::INSERT);
            os << getFlagName(flag) << " exit " << toggle << AnsiOstream::endl;
        }
        m_self.applyChanges(changeList);
        send_ok(os);
    }

    void onModifyRoomFlag(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        // REVISIT: Some flags conflict with each other
        const Vector &flags = v[1].getVector();
        ChangeList changeList;
        for (const auto &flag : flags) {
            const Vector &vector = flag.getVector();
            const bool is_add = vector[0].getBool();

            const std::optional<RoomFieldVariant> opt = evalRoomField(vector[1].getString());
            if (!opt) {
                throw std::runtime_error("unable to select current room");
            }

            // Check if the variant holds an enum type and adjust mode
            const FlagModifyModeEnum mode = [is_add](const RoomFieldEnum type) {
                if (type == RoomFieldEnum::ALIGN_TYPE || type == RoomFieldEnum::LIGHT_TYPE
                    || type == RoomFieldEnum::RIDABLE_TYPE || type == RoomFieldEnum::PORTABLE_TYPE
                    || type == RoomFieldEnum::SUNDEATH_TYPE
                    || type == RoomFieldEnum::TERRAIN_TYPE) {
                    return is_add ? FlagModifyModeEnum::ASSIGN : FlagModifyModeEnum::CLEAR;
                } else {
                    return is_add ? FlagModifyModeEnum::INSERT : FlagModifyModeEnum::REMOVE;
                }
            }(opt.value().getType());

            changeList.add(Change{ModifyRoomFlags{roomId, opt.value(), mode}});

            const auto toggle = enabledString(is_add);
            os << "Room flag " << toggle << AnsiOstream::endl;
        }

        m_self.applyChanges(changeList);
        send_ok(os);
    }

    void onAppendRoomNote(User &user, const Vector &v) const
    {
        const auto &room = getRoom();
        auto &os = user.getOstream();

        if constexpr ((IS_DEBUG_BUILD)) {
            const auto &append = v[1].getString();
            assert(append == "append");
        }

        std::string note = concatenate_unquoted(v[2].getVector());
        sanitizeRoomNote(note);

        if (note.empty()) {
            os << "What do you want to append to the note?\n";
            return;
        }

        const RoomNote roomNote = appendNote(room.getNote(), note);

        if (roomNote.empty()) {
            os << "Error: That's an empty string.\n";
            return;
        }

        m_self.applySingleChange(Change{
            ModifyRoomFlags{room.getId(), RoomFieldVariant{roomNote}, FlagModifyModeEnum::ASSIGN}});

        // REVISIT: show them the diff?
        os << "Note: " << roomNote.toStdStringUtf8() << "\n";
        send_ok(os);
    }

    void onClearRoomNote(User &user, const Vector & /*v*/) const
    {
        const auto roomId = getRoomId();

        auto &os = user.getOstream();
        m_self.applySingleChange(Change{
            ModifyRoomFlags{roomId, RoomFieldVariant{RoomNote{}}, FlagModifyModeEnum::CLEAR}});
        send_ok(os);
    }

    void onSetRoomNote(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        if constexpr (IS_DEBUG_BUILD) {
            const auto &set = v[1].getString();
            assert(set == "set");
        }

        std::string note = concatenate_unquoted(v[2].getVector());
        sanitizeRoomNote(note);
        if (note.empty()) {
            os << "What do you want to set the note to?\n";
            return;
        }

        const auto before = getMap().getRoomHandle(roomId).getNote();
        const auto desired = makeRoomNote(note);

        m_self.applySingleChange(
            Change{ModifyRoomFlags{roomId, RoomFieldVariant{desired}, FlagModifyModeEnum::ASSIGN}});

        const auto after = getMap().getRoomHandle(roomId).getNote();

        qInfo() << mmqt::toQByteArrayUtf8(note);
        qInfo() << before.toQByteArray();
        qInfo() << desired.toQByteArray();
        qInfo() << after.toQByteArray();

        // REVISIT: show them the diff?
        os << "Note: " << after.toStdStringUtf8() << "\n";
        send_ok(os);
    }

    void onSetRoomName(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        if constexpr (IS_DEBUG_BUILD) {
            const auto &name = v[1].getString();
            assert(name == "name");
        }

        std::string name = v[2].getString();
        sanitizeRoomName(name);

        if (!name.empty()) {
            for (const char c : name) {
                if ((ascii::isSpace(c) && c != char_consts::C_SPACE) || c == char_consts::C_NBSP) {
                    os << "Room name cannot contain non-standard whitespace.\n";
                    return;
                } else if (std::iscntrl(c)) {
                    os << "Room name cannot control codes.\n";
                    return;
                }
            }

            StringView sv{name};
            sv.trim();
            name = sv.toStdString();
            sanitizeRoomName(name);

            if (name.empty()) {
                os << "Room name must contain more than just whitespace.\n";
                return;
            }
        }

        m_self.applySingleChange(Change{
            ModifyRoomFlags{roomId, RoomFieldVariant{RoomName{name}}, FlagModifyModeEnum::ASSIGN}});

        if (name.empty()) {
            os << "Name removed.\n";
        } else {
            send_ok(os);
        }
    }

    void onSetServerId(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        if constexpr (IS_DEBUG_BUILD) {
            const auto &name = v[1].getString();
            assert(name == "server_id");
        }

        const int n = v[2].getInt();
        if (n < 0) {
            os << "Server id cannot be negative.";
            return;
        }

        const auto serverId = ServerRoomId{static_cast<uint32_t>(n)};
        m_self.applySingleChange(Change{room_change_types::SetServerId{roomId, serverId}});

        if (serverId == INVALID_SERVER_ROOMID) {
            os << "Server id removed.\n";
        } else {
            send_ok(os);
        }
    }

    void onNoexit(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        const auto dir = v[0].getDirection();
        // REVISIT: actually pass the ExternalRoomId?
        // Except then we'd need it to be either "any" or an id.
        const auto other = v[1].getInt();

        if (!hasExit(dir)) {
            os << "There is no exit " << to_string_view(dir) << ".\n";
            return;
        }

        const auto &here = getRoom();

        // -1 is a hack for "all"
        if (other == -1) {
            m_self.applySingleChange(
                Change{exit_change_types::NukeExit{roomId, dir, WaysEnum::OneWay}});
            os << "Removed all exits " << to_string_view(dir) << " (see diff for details).\n";
        } else {
            const auto otherId = m_self.getOtherRoom(other);
            if (!here.getExit(dir).containsOut(otherId)) {
                os << "There is no exit " << to_string_view(dir) << " to " << other << ".\n";
                return;
            }
            m_self.applySingleChange(Change{exit_change_types::ModifyExitConnection{
                ChangeTypeEnum::Remove, roomId, dir, otherId, WaysEnum::OneWay}});
            send_ok(os);
        }
    }

    void onNoEntrance(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        const auto dir = v[0].getDirection();
        // REVISIT: actually pass the ExternalRoomId?
        // Except then we'd need it to be either "any" or an id.
        const auto other = v[1].getInt();

        if (!hasEntrance(dir)) {
            os << "There is no exit " << to_string_view(dir) << ".\n";
            return;
        }

        const auto &here = getRoom();

        // -1 is a hack for "all"
        if (other == -1) {
            ChangeList changes;
            const auto &exit = here.getExit(dir);
            const ExitDirEnum rev = opposite(dir);
            for (const RoomId from : exit.getIncomingSet()) {
                changes.add(Change{exit_change_types::ModifyExitConnection{ChangeTypeEnum::Remove,
                                                                           from,
                                                                           rev,
                                                                           roomId,
                                                                           WaysEnum::OneWay}});
            }
            m_self.applyChanges(changes);
            os << "Removed all entrances " << to_string_view(dir) << " (see diff for details).\n";
        } else {
            const auto otherId = m_self.getOtherRoom(other);
            if (!here.getExit(dir).containsIn(otherId)) {
                os << "There is no entrance " << to_string_view(dir) << " from " << other << ".\n";
                return;
            }
            m_self.applySingleChange(Change{exit_change_types::ModifyExitConnection{
                ChangeTypeEnum::Remove, otherId, opposite(dir), roomId, WaysEnum::OneWay}});
            send_ok(os);
        }
    }

    void onAddExit(User &user, const Vector &v, WaysEnum ways) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        const auto dir = v[0].getDirection();
        const int otherRoomId = v[1].getInt();
        if (otherRoomId < 0) {
            os << "RoomId cannot be negative.\n";
            return;
        }
        const auto otherExt = ExternalRoomId{static_cast<uint32_t>(otherRoomId)};

        auto &mapData = getMap();
        auto other = mapData.findRoomHandle(otherExt);
        if (!other) {
            os << "To what RoomId?\n";
            return;
        }

        assert(otherExt == other.getIdExternal());
        m_self.applySingleChange(Change{exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                                                roomId,
                                                                                dir,
                                                                                other.getId(),
                                                                                ways}});

        send_ok(os);
    }

    void onStatFn(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        if constexpr (IS_DEBUG_BUILD) {
            const auto &stat = v[0].getString();
            assert(stat == "stat");
        }

        getMap().getCurrentMap().statRoom(os, roomId);
    }

    void onDiffFn(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        if constexpr (IS_DEBUG_BUILD) {
            const auto &diff = v[0].getString();
            assert(diff == "diff");
        }

        auto &mapData = getMap();
        const auto &baseMap = mapData.getSavedMap();
        const auto &currentMap = mapData.getCurrentMap();

        const auto &thisRoom = currentMap.getRoomHandle(roomId);
        const ExternalRoomId extId = thisRoom.getIdExternal();
        const auto before = baseMap.findRoomHandle(extId);
        if (!before) {
            os << "Room " << extId.value() << " has been added since the last save:\n";
            os << "\n";
            OstreamDiffReporter odr{os};
            odr.added(thisRoom);
            os << "\n";
            os << "End of changes.\n";
            return;
        }

        std::ostringstream oss;
        {
            AnsiOstream aos{oss};
            OstreamDiffReporter odr{aos};
            compare(odr, before, thisRoom);
        }

        auto str = std::move(oss).str();
        if (str.empty()) {
            os << "No changes in room " << extId.value() << " since the last save.\n";
        } else {
            os << "Changes in room " << extId.value() << " since the last save:\n";
            os << "\n";
            os.writeWithEmbeddedAnsi(str);
            os << "\n";
            os << "End of changes.\n";
        }
    }

    void onRevertFn(User &user, const Vector &v) const
    {
        const auto roomId = getRoomId();
        auto &os = user.getOstream();

        if constexpr (IS_DEBUG_BUILD) {
            assert(v.empty());
        }

        auto &mapData = getMap();
        const auto &baseMap = mapData.getSavedMap();
        const auto &currentMap = mapData.getCurrentMap();
        auto pResult = room_revert::build_plan(os, currentMap, roomId, baseMap);
        if (!pResult) {
            return;
        }

        const room_revert::RevertPlan &plan = deref(pResult);
        const ChangeList &changes = plan.changes;

        if (plan.warnNoEntrances) {
            os << "Note: Entrances will not be modified; however, you can manually use one of the"
               << " `exit`, `noexit`, or `noentrance` sub-commands to manually update a single"
               << " one-way exit,  or you can use the `dig` sub-command to create a two-way exit.\n";
        }

        if (changes.empty()) {
            os << "No changes will be made.\n";
            return;
        }

        if (false) {
            os << "The following change-requests will be applied:\n";
            ChangePrinter printer{[&currentMap](RoomId id) -> ExternalRoomId {
                                      return currentMap.getExternalRoomId(id);
                                  },
                                  os};
            for (const auto &change : changes.getChanges()) {
                os << " * ";
                printer.accept(change);
                os << "\n";
            }
        }

        auto &map = getMap();
        if (!map.applyChanges(changes)) {
            os << "Ooops... Something went wrong?\n";
            return;
        }

        const auto &now = map.getRawRoom(roomId);
        const bool partial = (now != plan.expect);
        os << "Success: The room has been " << (partial ? "partially" : "fully") << " restored.\n";

        // TODO: implement try-undelete.
        if (false && partial && plan.hintUndelete) {
            os << "Hint: Use `" << getPrefixChar()
               << "map try-undelete <id>` to try to restore a room that that has been removed.\n";
        }
    }

private:
    NODISCARD auto processHiddenParam(syntax::Accept::Function2 fn, std::string help)
    {
        return syntax::Accept::convert(
            [this, callback = std::move(fn)](User &u, const Vector &argv) {
                if (argv.empty()) {
                    throw std::runtime_error("wrong number of arguments");
                }

                // argv[0] is the optional room
                m_roomId = m_self.getOptionalOtherRoom(argv, 0);

                // copy the reset of the vector
                const Vector v2{std::vector<Value>{argv.begin() + 1, argv.end()}};
                callback(u, v2);
            },
            std::move(help));
    }

private:
    NODISCARD syntax::SharedConstSublist createSyntax()
    {
        using namespace ::syntax;
        static const auto abb = syntax::abbrevToken;

        auto printDynamic = processHiddenParam([this](User &u, const Vector &argv)
                                                   -> void { onPrintDynamic(u, argv); },
                                               "print room dynamic");
        auto printStatic = processHiddenParam([this](User &u, const Vector &argv)
                                                  -> void { onPrintStatic(u, argv); },
                                              "print room static");
        auto printNote = processHiddenParam([this](User &u, const Vector &argv)
                                                -> void { onPrintNote(u, argv); },
                                            "print room note");
        auto previewRoom = processHiddenParam([this](User &u, const Vector &argv)
                                                  -> void { onPreviewRoom(u, argv); },
                                              "print an offline preview");

        auto printSyntax = buildSyntax(abb("print"),
                                       buildSyntax(abb("dynamic"), printDynamic),
                                       buildSyntax(abb("static"), printStatic),
                                       buildSyntax(abb("note"), printNote),
                                       buildSyntax(abb("offline-preview"), previewRoom));

        auto modifyDoorFlag
            = processHiddenParam([this](User &u, const Vector &argv) { onModifyDoorFlag(u, argv); },
                                 "modify door flag");

        auto setDoorName = processHiddenParam([this](User &u,
                                                     const Vector &argv) { onSetDoorName(u, argv); },
                                              "modify door name");

        auto clearDoorName
            = processHiddenParam([this](User &u, const Vector &argv) { onClearDoorName(u, argv); },
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

        auto modifyExitFlag
            = processHiddenParam([this](User &u, const Vector &argv) { onModifyExitFlag(u, argv); },
                                 "modify exit flag");

        auto exitFlagsSyntax = buildSyntax(abb("exit-flags"),
                                           TokenMatcher::alloc<ArgDirection>(),
                                           buildSyntax(TokenMatcher::alloc<ArgOneOrMoreToken>(
                                                           TokenMatcher::alloc<ArgExitFlag>()),
                                                       modifyExitFlag));

        auto modifyRoomFlag
            = processHiddenParam([this](User &u, const Vector &argv) { onModifyRoomFlag(u, argv); },
                                 "modify room flag");

        auto flagsSyntax = buildSyntax(abb("flags"),
                                       buildSyntax(TokenMatcher::alloc<ArgOneOrMoreToken>(
                                                       TokenMatcher::alloc<ArgRoomFlag>()),
                                                   modifyRoomFlag));

        auto appendRoomNote
            = processHiddenParam([this](User &u, const Vector &argv) { onAppendRoomNote(u, argv); },
                                 "append room note");

        auto clearRoomNote
            = processHiddenParam([this](User &u, const Vector &argv) { onClearRoomNote(u, argv); },
                                 "clear room note");

        auto setRoomNote = processHiddenParam([this](User &u,
                                                     const Vector &argv) { onSetRoomNote(u, argv); },
                                              "set room note");

        auto setRoomName = processHiddenParam([this](User &u,
                                                     const Vector &argv) { onSetRoomName(u, argv); },
                                              "set room name");
        auto setServerId = processHiddenParam([this](User &u,
                                                     const Vector &argv) { onSetServerId(u, argv); },
                                              "set server id");

        auto noexit = processHiddenParam([this](User &u, const Vector &argv) { onNoexit(u, argv); },
                                         "remove an exit (or -1 for all exits)");

        auto noentrance = processHiddenParam([this](User &u,
                                                    const Vector &argv) { onNoEntrance(u, argv); },
                                             "remove an entrance (or -1 for all entrances)");

        auto makeConn = [this](WaysEnum ways) {
            return [this, ways](User &u, const Vector &argv) { onAddExit(u, argv, ways); };
        };

        auto statFn = processHiddenParam([this](User &u, const Vector &argv) { onStatFn(u, argv); },
                                         "display the current room stats");

        auto diffFn = processHiddenParam([this](User &u, const Vector &argv) { onDiffFn(u, argv); },
                                         "print the changes since the last save");

        auto revertFn = processHiddenParam([this](User &u,
                                                  const Vector &argv) { onRevertFn(u, argv); },
                                           "attempt to revert the changes since the last save");

        auto argInt = TokenMatcher::alloc<ArgInt>();
        auto argOptInt = TokenMatcher::alloc_copy(syntax::ArgOptionalToken(argInt));

        auto noteSyntax
            = buildSyntax(abb("note"),
                          buildSyntax(abb("append"), TokenMatcher::alloc<ArgRest>(), appendRoomNote),
                          buildSyntax(abb("clear"), clearRoomNote),
                          buildSyntax(abb("set"), TokenMatcher::alloc<ArgRest>(), setRoomNote));

        auto setSyntax
            = buildSyntax(abb("set"),
                          buildSyntax(abb("name"), TokenMatcher::alloc<ArgString>(), setRoomName),
                          buildSyntax(abb("server_id"), TokenMatcher::alloc<ArgInt>(), setServerId));

        auto makeExitSyn = [this, &makeConn](WaysEnum ways, std::string name, std::string desc) {
            return buildSyntax(stringToken(std::move(name)),
                               TokenMatcher::alloc<ArgDirection>(),
                               TokenMatcher::alloc<ArgInt>(),
                               processHiddenParam(makeConn(ways), std::move(desc)));
        };

        auto exitSyntax = makeExitSyn(WaysEnum::OneWay, "exit", "make a 1-way connection");
        auto digSyntax = makeExitSyn(WaysEnum::TwoWay, "dig", "make a 2-way connection");

        auto noexitSyntax = buildSyntax(stringToken("noexit"),
                                        TokenMatcher::alloc<ArgDirection>(),
                                        argInt,
                                        noexit);

        auto noentranceSyntax = buildSyntax(stringToken("noentrance"),
                                            TokenMatcher::alloc<ArgDirection>(),
                                            argInt,
                                            noentrance);

        auto statSyntax = buildSyntax(abb("stat"), statFn);
        auto diffSyntax = buildSyntax(abb("diff"), diffFn);
        auto revertSyntax = buildSyntax(stringToken("revert"), revertFn);

        auto selectRoom = processHiddenParam(
            [this](User &user, const Vector &argv) {
                auto &os = user.getOstream();
                assert(argv[0].getString() == "select");
                const auto room = getRoom();
                const auto tmpSel = RoomSelection::createSelection(RoomIdSet{room.getId()});
                m_self.onNewRoomSelection(SigRoomSelection{tmpSel});
                os << "Room ";
                os << room.getIdExternal().asUint32();
                os << " has been temporarily selected on the map.\n";
                send_ok(os);
            },
            "select room");

        auto selectSyntax = buildSyntax(abb("select"), selectRoom);

        auto roomSyntax = buildSyntax(doorSyntax,
                                      exitFlagsSyntax,
                                      flagsSyntax,
                                      noteSyntax,
                                      printSyntax,
                                      setSyntax,
                                      exitSyntax,
                                      digSyntax,
                                      noexitSyntax,
                                      noentranceSyntax,
                                      diffSyntax,
                                      statSyntax,
                                      revertSyntax,
                                      selectSyntax);

        return roomSyntax;
    }

private:
    NODISCARD bool hasExit(const ExitDirEnum dir) const
    {
        return getRoom().getExit(dir).exitIsExit();
    }

    NODISCARD bool hasEntrance(const ExitDirEnum dir) const
    {
        return !getRoom().getExit(dir).getIncomingSet().empty();
    }

    NODISCARD bool hasDoor(const ExitDirEnum dir) const
    {
        return getRoom().getExit(dir).exitIsDoor();
    }

public:
    void eval(StringView input)
    {
        // NOTE: It's very important to reset this every time.
        m_roomId = INVALID_ROOMID;

        using namespace syntax;
        const auto thisCommand = std::string(1, getPrefixChar()).append("room");
        const auto completeSyntax = buildSyntax(stringToken(thisCommand),
                                                TokenMatcher::alloc<ArgOptionalToken>(
                                                    TokenMatcher::alloc<ArgInt>()),
                                                m_syntax);

        // TODO: output directly to user's ostream instead of returning a string.
        auto result = processSyntax(completeSyntax, thisCommand, input);
        m_self.sendToUser(SendToUserSourceEnum::FromMMapper, result);
    }
};

void AbstractParser::parseRoom(StringView input)
{
    if (m_parseRoomHelper == nullptr) {
        m_parseRoomHelper = std::make_shared<ParseRoomHelper>(*this);
    }

    auto &helper = deref(m_parseRoomHelper);
    helper.eval(input);
}
