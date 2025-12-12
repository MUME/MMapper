// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/configuration.h"
#include "../display/InfomarkSelection.h"
#include "../global/AnsiOstream.h"
#include "../global/CaseUtils.h"
#include "../global/TextUtils.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFlags.h"
#include "../map/enums.h"
#include "../map/infomark.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
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

NODISCARD static const char *getTypeName(const InfomarkTypeEnum type)
{
#define CASE(UPPER, s) \
    do { \
    case InfomarkTypeEnum::UPPER: \
        return s; \
    } while (false)
    switch (type) {
        CASE(TEXT, "text");
        CASE(LINE, "line");
        CASE(ARROW, "arrow");
    }
    return "unknown";
#undef CASE
}

class NODISCARD ArgMarkClass final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgMarkClass::virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *logger) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);
    const auto arg = ::toLowerUtf8(input.front());
    StringView sv(arg);

    for (const auto &clazz : ::enums::getAllInfomarkClasses()) {
        const auto &command = getParserCommandName(clazz);
        if (!command.matches(sv)) {
            continue;
        }
        return syntax::MatchResult::success(1, input, Value(clazz));
    }

    if (logger) {
        std::ostringstream os;
        for (const auto &clazz : ::enums::getAllInfomarkClasses()) {
            os << getParserCommandName(clazz).getCommand() << " ";
        }
        logger->logError("input was not a valid mark class: " + os.str());
    }
    return syntax::MatchResult::failure(input);
}

std::ostream &ArgMarkClass::virt_to_stream(std::ostream &os) const
{
    return os << "<class>";
}

void AbstractParser::parseMark(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    auto getPositionCoordinate = [this]() -> Coordinate {
        // get scaled coordinates of room center.
        static_assert(INFOMARK_SCALE % 2 == 0);
        const Coordinate halfRoomOffset{INFOMARK_SCALE / 2, INFOMARK_SCALE / 2, 0};

        // do not scale the z-coordinate!  only x,y should get scaled.
        const auto opt_pos = m_mapData.tryGetPosition();
        if (!opt_pos) {
            throw std::runtime_error("current position is unknown");
        }
        const Coordinate pos = opt_pos.value();
        Coordinate c{pos.x * INFOMARK_SCALE, pos.y * INFOMARK_SCALE, pos.z};
        c += halfRoomOffset;
        return c;
    };

    auto getInfomarkSelection = [this](const Coordinate &c) -> std::shared_ptr<InfomarkSelection> {
        // the scaling + offset operation looks like `A*x + b` where A is a 3x3
        // transformation matrix and b,x are 3-vectors

        // A = [[INFOMARK_SCALE/2, 0                 0]
        //      [0,                INFOMARK_SCALE/2, 0]
        //      [0,                0,                1]]
        // b = halfRoomOffset
        // x = m_mapData->getPosition()
        //
        // c = A*x + b

        static_assert(INFOMARK_SCALE % 5 == 0);
        static constexpr auto INFOMARK_ROOM_RADIUS = INFOMARK_SCALE / 2;
        const auto lo = c + Coordinate{-INFOMARK_ROOM_RADIUS, -INFOMARK_ROOM_RADIUS, 0};
        const auto hi = c + Coordinate{+INFOMARK_ROOM_RADIUS, +INFOMARK_ROOM_RADIUS, 0};
        return InfomarkSelection::alloc(m_mapData, lo, hi);
    };

    auto listMark = Accept(
        [getPositionCoordinate, getInfomarkSelection](User &user, const Pair * /*args*/) {
            AnsiOstream &aos = user.getOstream();

            auto printCoordinate = [&aos](const Coordinate c) {
                aos << "(" << c.x << ", " << c.y << ", " << c.z << ")";
            };

            const Coordinate c = getPositionCoordinate();
            aos << "Marks near coordinate ";
            printCoordinate(c);
            aos << "\n";

            static const auto green = getRawAnsi(AnsiColor16Enum::GREEN);
            static const auto yellow = getRawAnsi(AnsiColor16Enum::YELLOW);

            int count = 0;
            std::shared_ptr<InfomarkSelection> is = getInfomarkSelection(c);
            is->for_each([&aos, &count, &printCoordinate](const InfomarkHandle &mark) {
                count += 1;
                aos << "Mark type: " << getTypeName(mark.getType()) << "\n";
                aos << "  id: ";
                aos.writeWithColor(green, mark.getId().value());
                aos << "\n";
                aos << "  angle: " << mark.getRotationAngle() << "\n";
                aos << "  class: " << getParserCommandName(mark.getClass()).getCommand() << "\n";
                if (mark.getType() == InfomarkTypeEnum::TEXT) {
                    aos << "  text: ";
                    aos.writeQuotedWithColor(green, yellow, mark.getText().getStdStringViewUtf8());
                    aos << "\n";
                } else {
                    aos << "  pos1: ";
                    printCoordinate(mark.getPosition1());
                    aos << "\n";
                    aos << "  pos2: ";
                    printCoordinate(mark.getPosition2());
                    aos << "\n";
                }
            });
            if (count == 0) {
                aos << "None.\n";
            } else {
                aos << "Total: " << count << "\n";
            }
        },
        "list marks");

    auto listSyntax = buildSyntax(abb("list"), listMark);

    auto lookup_mark = [this](int n) -> InfomarkId {
        if (n < 0) {
            throw std::runtime_error("invalid mark");
        }
        auto id = static_cast<InfomarkId>(static_cast<uint32_t>(n));
        const auto &map = m_mapData.getCurrentMap();
        auto mark = map.getInfomarkDb().find(id);
        if (!mark.exists()) {
            throw std::runtime_error("invalid mark");
        }
        return id;
    };

    auto removeMark = Accept(
        [this, getPositionCoordinate, getInfomarkSelection, lookup_mark](User &user,
                                                                         const Pair *args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "remove");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfomarkSelection> is = getInfomarkSelection(c);

            const auto id = lookup_mark(v[1].getInt());
            if (!m_mapData.applySingleChange(Change{infomark_change_types::RemoveInfomark{id}})) {
                os << "Unable to remove marker.\n";
                return;
            }
            send_ok(os);
        },
        "remove mark");

    auto removeSyntax = buildSyntax(abb("remove"),
                                    TokenMatcher::alloc_copy<ArgInt>(ArgInt::withMin(1)),
                                    removeMark);

    auto addRoomMark = Accept(
        [this, getPositionCoordinate](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "add");
            }

            const std::string text = concatenate_unquoted(v[1].getVector());
            if (text.empty()) {
                os << "What do you want to set the mark to?\n";
                return;
            }

            // create a text infomark above this room
            const Coordinate c = getPositionCoordinate();

            RawInfomark mark;
            mark.setType(InfomarkTypeEnum::TEXT);
            mark.setText(InfomarkText{text});
            mark.setClass(InfomarkClassEnum::COMMENT);
            mark.setPosition1(c);

            if (!m_mapData.applySingleChange(Change{infomark_change_types::AddInfomark{mark}})) {
                os << "Unable to add mark.\n";
                return;
            }

            send_ok(os);
        },
        "add mark");

    auto addSyntax = buildSyntax(abb("add"), TokenMatcher::alloc<ArgRest>(), addRoomMark);

    using Callback = std::function<void(RawInfomark & mark)>;
    auto modify_mark = [this](InfomarkId id, const Callback &callback) {
        const auto &map = m_mapData.getCurrentMap();
        const auto &db = map.getInfomarkDb();
        auto mark = db.getRawCopy(id);
        callback(mark);
        return this->m_mapData.applySingleChange(
            Change{infomark_change_types::UpdateInfomark{id, mark}});
    };

    auto modifyText = Accept(
        [this,
         getPositionCoordinate,
         getInfomarkSelection,
         lookup_mark,
         modify_mark](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "set");
                const auto &text = v[2].getString();
                assert(text == "text");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfomarkSelection> is = getInfomarkSelection(c);

            const auto id = lookup_mark(v[1].getInt());

            {
                const auto &map = m_mapData.getCurrentMap();
                const auto &db = map.getInfomarkDb();
                const auto mark = db.getRawCopy(id);
                if (mark.getType() != InfomarkTypeEnum::TEXT) {
                    throw std::runtime_error("unable to set text to this mark");
                }
            }

            // update text of the first existing text infomark in this room
            const std::string text = concatenate_unquoted(v[3].getVector());
            if (text.empty()) {
                os << "What do you want to set the mark's text to?\n";
                return;
            }

            if (modify_mark(id, [text](RawInfomark &mark) { mark.setText(InfomarkText{text}); })) {
                send_ok(os);
            } else {
                os << "Error setting mark text.\n";
            }
        },
        "modify mark text");

    auto modifyClass = Accept(
        [lookup_mark, modify_mark](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "set");
                const auto &clazz = v[2].getString();
                assert(clazz == "class");
            }

            const auto id = lookup_mark(v[1].getInt());
            const InfomarkClassEnum clazz = v[3].getInfomarkClass();
            if (modify_mark(id, [clazz](RawInfomark &mark) { mark.setClass(clazz); })) {
                send_ok(os);
            } else {
                os << "Error setting mark class.\n";
            }
        },
        "modify mark class");

    auto modifyAngle = Accept(
        [getPositionCoordinate,
         getInfomarkSelection,
         lookup_mark,
         modify_mark](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "set");
                const auto &angle = v[2].getString();
                assert(angle == "angle");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfomarkSelection> is = getInfomarkSelection(c);

            const auto id = lookup_mark(v[1].getInt());
            const int degrees = v[3].getInt();
            if (modify_mark(id, [degrees](RawInfomark &mark) { mark.setRotationAngle(degrees); })) {
                send_ok(os);
            } else {
                os << "Error setting mark angle.\n";
            }
        },
        "modify mark angle");

    // REVISIT: Does it make sense to allow user to change the type to arrow or line? What about position?
    auto setSyntax
        = buildSyntax(abb("set"),
                      TokenMatcher::alloc_copy<ArgInt>(ArgInt::withMin(1)),
                      buildSyntax(abb("angle"),
                                  TokenMatcher::alloc_copy<ArgInt>(ArgInt::withMinMax(0, 360)),
                                  modifyAngle),
                      buildSyntax(abb("class"), TokenMatcher::alloc<ArgMarkClass>(), modifyClass),
                      buildSyntax(abb("text"), TokenMatcher::alloc<ArgRest>(), modifyText));

    auto markSyntax = buildSyntax(addSyntax, listSyntax, removeSyntax, setSyntax);

    eval("mark", markSyntax, input);
}
