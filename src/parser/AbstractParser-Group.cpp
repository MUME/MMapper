// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/PrintUtils.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <iostream>
#include <ostream>

class NODISCARD ArgMember final : public syntax::IArgument
{
private:
    GroupManagerApi &m_group;

public:
    explicit ArgMember(GroupManagerApi &group)
        : m_group(group)
    {}

private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgMember::virt_match(const syntax::ParserInput &input,
                                          syntax::IMatchErrorLogger *logger) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    static_assert(std::is_same_v<const std::string &, decltype(input.front())>);

    SharedGroupChar member;
    const std::string &str = input.front();
    if (!str.empty() && std::all_of(str.begin(), str.end(), ::isdigit)) {
        const auto id = GroupId{static_cast<uint32_t>(std::stoul(str))};
        member = m_group.getMember(id);
    } else {
        member = m_group.getMember(CharacterName{str});
    }

    if (member) {
        return syntax::MatchResult::success(1,
                                            input,
                                            Value{static_cast<int64_t>(member->getId().asUint32())});
    }

    if (logger) {
        std::ostringstream os;
        bool comma = false;
        for (const auto &ch : m_group.getMembers()) {
            if (comma) {
                os << ", ";
            }
            os << ch->getName().getStdStringViewUtf8() << " (" << ch->getId().asUint32() << ")";
            comma = true;
        }
        logger->logError("input was not a valid group member: " + os.str());
    }
    return syntax::MatchResult::failure(input);
}

std::ostream &ArgMember::virt_to_stream(std::ostream &os) const
{
    return os << "<name|id>";
}

void AbstractParser::parseGroup(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    const auto optArgEquals = TokenMatcher::alloc<ArgOptionalChar>(char_consts::C_EQUALS);

    auto listColors = syntax::Accept(
        [this](User &user, const Pair *) {
            auto &os = user.getOstream();

            const auto &members = m_group.getMembers();
            if (members.empty()) {
                os << "no group members found";
                return;
            }

            os << "Customizable colors:\n";
            char buf[3];
            for (const auto &member : members) {
                snprintf(buf, sizeof(buf), "%2d", static_cast<int32_t>(member->getId().asUint32()));
                const auto name = member->getName().getStdStringViewUtf8();
                const auto color = Color(member->getColor());
                os << buf << " " << name << " = " << color << AnsiOstream::endl;
            }
        },
        "list group member ids and colors");

    auto setMemberColor = syntax::Accept(
        [this](User &user, const Pair *args) {
            auto &os = user.getOstream();
            if (args == nullptr || args->cdr == nullptr || !args->car.isLong()
                || !args->cdr->car.isLong()) {
                throw std::runtime_error("internal error");
            }

            const auto id = static_cast<uint32_t>(args->cdr->car.getLong());
            const auto rgb = static_cast<uint32_t>(args->car.getLong());

            const auto member = m_group.getMember(GroupId{id});
            if (!member) {
                throw std::runtime_error("internal error");
            }
            const auto name = member->getName().getStdStringViewUtf8();

            const auto oldColor = Color(member->getColor());
            const auto newColor = Color::fromRGB(rgb);

            if (oldColor.getRGB() == rgb) {
                os << "Member " << name << " (" << id << ") is already " << newColor << ".\n";
                return;
            }

            member->setColor(newColor.getQColor());
            if (member->isYou()) {
                setConfig().groupManager.color = member->getColor();
            }
            os << "Member " << name << " (" << id << ") has been changed from " << oldColor
               << " to " << newColor << ".\n";

            m_group.refresh();
        },
        "set group member color");

    auto listSyntax = buildSyntax(abb("list"), listColors);

    auto setSyntax = buildSyntax(abb("set"),
                                 TokenMatcher::alloc<ArgMember>(m_group),
                                 optArgEquals,
                                 TokenMatcher::alloc<ArgHexColor>(),
                                 setMemberColor);

    auto colorsSyntax = buildSyntax(listSyntax, setSyntax);

    eval("group", colorsSyntax, input);
}
