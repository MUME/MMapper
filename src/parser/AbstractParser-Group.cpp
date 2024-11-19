// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/configuration.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <ostream>
#include <sstream>

#include <QByteArray>

NODISCARD static QByteArray simplify(const std::string &s)
{
    return QByteArray::fromStdString(s).simplified();
}

class NODISCARD ArgPlayer final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgPlayer::virt_match(const syntax::ParserInput &input,
                                          syntax::IMatchErrorLogger *) const
{
    if (input.size() != 1) {
        return syntax::MatchResult::failure(input);
    }

    // REVISIT: Verify this player exists. Check for spaces?
    const std::string &name = input.front();
    return syntax::MatchResult::success(1, input, Value{name});
}

std::ostream &ArgPlayer::virt_to_stream(std::ostream &os) const
{
    return os << "<player>";
}

void AbstractParser::parseGroup(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    auto groupKickSyntax = [this]() -> SharedConstSublist {
        const auto argPlayer = TokenMatcher::alloc<ArgPlayer>();
        auto acceptKick = Accept(
            [this](User &user, const Pair *const args) -> void {
                auto &os = user.getOstream();
                const auto v = getAnyVectorReversed(args);
                if constexpr ((IS_DEBUG_BUILD)) {
                    const auto &kick = v[0].getString();
                    assert(kick == "kick");
                }
                const auto name = simplify(v[1].getString());

                if (name.isEmpty()) {
                    os << "Who do you want to kick?" << std::endl;
                    return;
                }

                m_group.kickCharacter(name);
                send_ok(os);
            },
            "kick group member");
        return buildSyntax(abb("kick"), argPlayer, acceptKick);
    }();

    auto groupLockSyntax = []() -> SharedConstSublist {
        const auto argBool = TokenMatcher::alloc<ArgBool>();
        auto acceptLock = Accept(
            [](User &user, const Pair *args) -> void {
                const auto value = deref(args).car.getBool();
                auto &os = user.getOstream();

                const auto msg = (value ? "locked" : "unlocked");
                bool &lockGroup = setConfig().groupManager.lockGroup;
                if (value == lockGroup) {
                    os << "Group was already " << msg << std::endl;
                    return;
                }

                lockGroup = value;
                os << "Group has been " << msg << std::endl;
            },
            "modify group lock");
        return buildSyntax(abb("lock"), argBool, acceptLock);
    }();

    auto groupTellSyntax = [this]() -> SharedConstSublist {
        const auto argRest = TokenMatcher::alloc<ArgRest>();
        auto acceptTell = Accept(
            [this](User &user, const Pair *const args) -> void {
                auto &os = user.getOstream();
                const auto v = getAnyVectorReversed(args);

                if constexpr ((IS_DEBUG_BUILD)) {
                    const auto &tell = v[0].getString();
                    assert(tell == "tell");
                }

                const auto message = simplify(concatenate_unquoted(v[1].getVector()));
                if (message.isEmpty()) {
                    os << "What do you want to tell the group?" << std::endl;
                    return;
                }

                m_group.sendGroupTell(message);
                send_ok(os);
            },
            "send group tell");
        return buildSyntax(abb("tell"), argRest, acceptTell);
    }();

    const auto groupSyntax = buildSyntax(groupKickSyntax, groupLockSyntax, groupTellSyntax);

    eval("group", groupSyntax, input);
}

void AbstractParser::sendScoreLineEvent(const QByteArray &arr)
{
    m_group.sendScoreLineEvent(arr);
}

void AbstractParser::sendPromptLineEvent(const QByteArray &arr)
{
    m_group.sendPromptLineEvent(arr);
}
