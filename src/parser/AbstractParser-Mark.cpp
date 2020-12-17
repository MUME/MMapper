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
#include "../display/InfoMarkSelection.h"
#include "../expandoracommon/room.h"
#include "../global/TextUtils.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/customaction.h"
#include "../mapdata/enums.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Commands.h"
#include "AbstractParser-Utils.h"

NODISCARD static const char *getTypeName(const InfoMarkTypeEnum type)
{
#define CASE(UPPER, s) \
    do { \
    case InfoMarkTypeEnum::UPPER: \
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
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgMarkClass::virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *logger) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    const auto arg = toLowerLatin1(input.front());
    StringView sv(arg);

    for (const auto &clazz : ::enums::getAllInfoMarkClasses()) {
        const auto &command = getParserCommandName(clazz);
        if (!command.matches(sv))
            continue;
        return syntax::MatchResult::success(1, input, Value(clazz));
    }

    if (logger) {
        std::ostringstream os;
        for (const auto &clazz : ::enums::getAllInfoMarkClasses())
            os << getParserCommandName(clazz).getCommand() << " ";
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
        const Coordinate pos = m_mapData.getPosition();
        Coordinate c{pos.x * INFOMARK_SCALE, pos.y * INFOMARK_SCALE, pos.z};
        c += halfRoomOffset;
        return c;
    };

    auto getInfoMarkSelection = [this](const Coordinate &c) -> std::shared_ptr<InfoMarkSelection> {
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
        return InfoMarkSelection::alloc(m_mapData, lo, hi);
    };

    auto listMark = Accept(
        [getPositionCoordinate, getInfoMarkSelection](User &user, const Pair * /*args*/) {
            auto &os = user.getOstream();

            auto printCoordinate = [&os](const Coordinate c) {
                os << "(" << c.x << ", " << c.y << ", " << c.z << ")";
            };

            const Coordinate c = getPositionCoordinate();
            os << "Marks near coordinate ";
            printCoordinate(c);
            os << std::endl;

            int n = 0;
            std::shared_ptr<InfoMarkSelection> is = getInfoMarkSelection(c);
            for (const auto &mark : *is) {
                if (n != 0)
                    os << std::endl;
                os << "\x1b[32m" << ++n << "\x1b[0m: " << getTypeName(mark->getType()) << std::endl;
                os << "  angle: " << mark->getRotationAngle() << std::endl;
                os << "  class: " << getParserCommandName(mark->getClass()).getCommand()
                   << std::endl;
                if (mark->getType() == InfoMarkTypeEnum::TEXT)
                    os << "  text: " << mark->getText().getStdString() << std::endl;
                else {
                    os << "  pos1: ";
                    printCoordinate(mark->getPosition1());
                    os << std::endl;
                    os << "  pos2: ";
                    printCoordinate(mark->getPosition2());
                    os << std::endl;
                }
            }
        },
        "list marks");

    auto listSyntax = buildSyntax(abb("list"), listMark);

    auto removeMark = Accept(
        [this, getPositionCoordinate, getInfoMarkSelection](User &user, const Pair *args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "remove");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfoMarkSelection> is = getInfoMarkSelection(c);

            const auto index = static_cast<size_t>(v[1].getInt() - 1);
            assert(index >= 0);
            if (index >= is->size())
                throw std::runtime_error("unable to select mark");

            // delete the infomark
            const auto &mark = is->at(index);
            m_mapData.removeMarker(mark);

            emit sig_infoMarksChanged();
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

            auto mark = InfoMark::alloc(m_mapData);
            mark->setType(InfoMarkTypeEnum::TEXT);
            mark->setText(InfoMarkText{text});
            mark->setClass(InfoMarkClassEnum::COMMENT);
            mark->setPosition1(c);

            m_mapData.addMarker(mark);

            emit sig_infoMarksChanged();
            send_ok(os);
        },
        "add mark");

    auto addSyntax = buildSyntax(abb("add"), TokenMatcher::alloc<ArgRest>(), addRoomMark);

    auto modifyText = Accept(
        [this, getPositionCoordinate, getInfoMarkSelection](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "set");
                const auto &text = v[2].getString();
                assert(text == "text");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfoMarkSelection> is = getInfoMarkSelection(c);

            const auto index = static_cast<size_t>(v[1].getInt() - 1);
            assert(index >= 0);
            if (index >= is->size())
                throw std::runtime_error("unable to select mark");

            const auto &mark = is->at(index);
            if (mark->getType() != InfoMarkTypeEnum::TEXT)
                throw std::runtime_error("unable to set text to this mark");

            // update text of the first existing text infomark in this room
            const std::string text = concatenate_unquoted(v[3].getVector());
            if (text.empty()) {
                os << "What do you want to set the mark's text to?\n";
                return;
            }
            mark->setText(InfoMarkText{text});

            emit sig_infoMarksChanged();
            send_ok(os);
        },
        "modify mark text");

    auto modifyClass = Accept(
        [this, getPositionCoordinate, getInfoMarkSelection](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "set");
                const auto &clazz = v[2].getString();
                assert(clazz == "class");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfoMarkSelection> is = getInfoMarkSelection(c);

            const auto index = static_cast<size_t>(v[1].getInt() - 1);
            assert(index >= 0);
            if (index >= is->size())
                throw std::runtime_error("unable to select mark");

            const auto clazz = v[3].getInfoMarkClass();
            const auto &mark = is->at(index);
            mark->setClass(clazz);

            emit sig_infoMarksChanged();
            send_ok(os);
        },
        "modify mark class");

    auto modifyAngle = Accept(
        [this, getPositionCoordinate, getInfoMarkSelection](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr (IS_DEBUG_BUILD) {
                const auto &set = v[0].getString();
                assert(set == "set");
                const auto &angle = v[2].getString();
                assert(angle == "angle");
            }

            const Coordinate c = getPositionCoordinate();
            std::shared_ptr<InfoMarkSelection> is = getInfoMarkSelection(c);

            const auto index = static_cast<size_t>(v[1].getInt() - 1);
            assert(index >= 0);
            if (index >= is->size())
                throw std::runtime_error("unable to select mark");

            const auto &mark = is->at(index);
            if (mark->getType() != InfoMarkTypeEnum::TEXT)
                throw std::runtime_error("unable to set angle to this mark");

            const auto angle = v[3].getInt();
            mark->setRotationAngle(angle);

            emit sig_infoMarksChanged();
            send_ok(os);
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
