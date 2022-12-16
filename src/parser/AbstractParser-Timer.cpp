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


class NODISCARD ArgTimerName final : public syntax::IArgument
{
private:
    syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                   syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgTimerName::virt_match(const syntax::ParserInput &input,
                                          syntax::IMatchErrorLogger *logger) const
{
    return syntax::MatchResult::success(1, input, Value{input.front()});
}

std::ostream &ArgTimerName::virt_to_stream(std::ostream &os) const
{
    return os << "<timers name>";
}

void AbstractParser::parseTimer(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    auto countdownTimer = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const auto name = v[0].getString();
            const auto delay = v[1].getInt();

            const std::string text = concatenate_unquoted(v[2].getVector());
            if (text.empty()) {
                os << "What do you want to set the timer for?\n";
                return;
            }

            m_timers.addCountdown(name.c_str(), text.c_str(), delay * 1000);
            sendToUser(
                QString("--[ Countdown  %1 <%2> is set for duration of %3 seconds")
                    .arg(QString::fromStdString(name), QString::fromStdString(text))
                    .arg(delay));

            sendToUser("\n");
            send_ok(os);
        },
        "configure countdown timer");

    auto countdownTimerSyntax =
        buildSyntax(
        stringToken("countdown"),
        buildSyntax(TokenMatcher::alloc<ArgTimerName>(), TokenMatcher::alloc<ArgInt>(), TokenMatcher::alloc<ArgRest>(), countdownTimer)
    );

    eval("timer", countdownTimerSyntax, input);
}
