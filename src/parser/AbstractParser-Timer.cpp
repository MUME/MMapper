// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "abstractparser.h"

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"

class NODISCARD ArgTimerName final : public syntax::IArgument
{
private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgTimerName::virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger * /*logger */) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    return syntax::MatchResult::success(1, input, Value{input.front()});
}

std::ostream &ArgTimerName::virt_to_stream(std::ostream &os) const
{
    return os << "<timer name>";
}

void AbstractParser::parseTimer(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    auto addCountdownTimer = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr ((IS_DEBUG_BUILD)) {
                const auto &append = v[1].getString();
                assert(append == "countdown");
            }

            const auto name = v[2].getString();
            const auto delay = v[3].getInt();

            const std::string desc = concatenate_unquoted(v[4].getVector());

            m_timers.addCountdown(name, desc, delay * 1000);
            os << "Added countdown timer " << name;
            if (!desc.empty())
                os << " <" << desc << ">";
            os << " for the duration of " << delay << " seconds." << std::endl;
            send_ok(os);
        },
        "add countdown timer");

    auto addSimpleTimer = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            if constexpr ((IS_DEBUG_BUILD)) {
                const auto &append = v[1].getString();
                assert(append == "simple");
            }

            const auto name = v[2].getString();
            const std::string desc = concatenate_unquoted(v[3].getVector());

            m_timers.addTimer(name, desc);
            os << "Added simple timer " << name;
            if (!desc.empty())
                os << " <" << desc << ">";
            os << "." << std::endl;
            send_ok(os);
        },
        "add simple timer");

    auto removeTimer = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const auto name = v[1].getString();

            auto res = false;
            if ((res = m_timers.removeTimer(name))) {
                os << "Removed simple timer " << name << "." << std::endl;
            } else if ((res = m_timers.removeCountdown(name))) {
                os << "Removed countdown timer " << name << "." << std::endl;
            } else {
                os << "No timer with that name found." << std::endl;
            }
            send_ok(os);
        },
        "remove timer");

    auto clearTimers = Accept(
        [this](User &user, const Pair *const /* args */) {
            auto &os = user.getOstream();
            m_timers.clear();
            os << "Cleared all timers." << std::endl;
            send_ok(os);
        },
        "clear all timers");

    auto listTimers = Accept(
        [this](User &user, const Pair *const /* args */) {
            auto &os = user.getOstream();
            const auto list = m_timers.getStatCommandEntry();
            if (list.empty())
                os << "No timers have been created yet." << std::endl;
            else
                os << list;
            send_ok(os);
        },
        "list all timers");

    auto addSimpleSyntax = buildSyntax(abb("simple"),
                                       TokenMatcher::alloc<ArgTimerName>(),
                                       TokenMatcher::alloc<ArgRest>(),
                                       addSimpleTimer);

    auto addCountdownSyntax = buildSyntax(abb("countdown"),
                                          TokenMatcher::alloc<ArgTimerName>(),
                                          TokenMatcher::alloc_copy<ArgInt>(
                                              ArgInt::withMinMax(1, 86400)),
                                          TokenMatcher::alloc<ArgRest>(),
                                          addCountdownTimer);

    auto addSyntax = buildSyntax(abb("add"), addCountdownSyntax, addSimpleSyntax);

    auto removeSyntax = buildSyntax(abb("remove"), TokenMatcher::alloc<ArgTimerName>(), removeTimer);

    auto clearSyntax = buildSyntax(abb("clear"), clearTimers);

    auto listSyntax = buildSyntax(abb("list"), listTimers);

    auto timerSyntax = buildSyntax(addSyntax, removeSyntax, listSyntax, clearSyntax);

    eval("timer", timerSyntax, input);
}
