// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../clock/mumeclock.h"
#include "../global/Consts.h"
#include "Action.h"
#include "abstractparser.h"

#include <cassert>
#include <memory>

// candidate for moving to utils?
template<typename Queue>
static void maybePop(Queue &queue)
{
    if (!queue.empty()) {
        queue.pop_front();
    }
}

void MumeXmlParserBase::initActionMap()
{
    auto &map = m_actionMap;
    map.clear();

    auto addStartsWith = [&map](const std::string &match, const ActionCallback &callback) {
        assert(!match.empty());
        const char hint = match[0];
        map.emplace(hint, std::make_unique<StartsWithAction>(match, callback));
    };

    auto addEndsWith = [&map](const std::string &match, const ActionCallback &callback) {
        assert(!match.empty());
        const char hint = 0;
        map.emplace(hint, std::make_unique<EndsWithAction>(match, callback));
    };

    auto addRegex = [&map](const std::string &match, const ActionCallback &callback) {
        assert(!match.empty());
        const char hint = [&match]() -> char {
            using namespace char_consts;
            if (match.length() > 2 && match[0] == C_CARET && match[1] != C_BACKSLASH
                && match[1] != C_OPEN_PARENS) {
                return match[1];
            }
            return 0;
        }();
        map.emplace(hint, std::make_unique<RegexAction>(match, callback));
    };

    /// Positions
    auto dead = [this](StringView /*view*/) {
        // REVISIT: send an event that the player died,
        // instead of trying to dig into details that are likely to get out of date?
        m_commonData.queue.clear();
        pathChanged();
        onReleaseAllPaths();

        // Highlight the current room
        const auto tmpSel = RoomSelection::createSelection(RoomIdSet{getTailPosition()});
        onNewRoomSelection(SigRoomSelection{tmpSel});
    };
    addStartsWith("You are dead!", dead);

    /// Path Machine: Prespam
    auto failedMovement = [this](StringView /*view*/) {
        maybePop(getQueue());
        pathChanged();
    };
    addStartsWith("You failed to climb", failedMovement);
    addStartsWith("You need to swim to go there.", failedMovement);
    addStartsWith("You cannot ride there.", failedMovement);
    addStartsWith("You are too exhausted.", failedMovement);
    addStartsWith("You are too exhausted to ride.", failedMovement);
    addStartsWith("Your mount refuses to follow your orders!", failedMovement);
    addStartsWith("You failed swimming there.", failedMovement);
    addStartsWith("You can't go into deep water!", failedMovement);
    addStartsWith("You cannot ride into deep water!", failedMovement);
    addStartsWith("You unsuccessfully try to break through the ice.", failedMovement);
    addStartsWith("Your boat cannot enter this place.", failedMovement);
    addStartsWith("Alas, you cannot go that way...", failedMovement);
    addStartsWith("No way! You are fighting for your life!", failedMovement);
    addStartsWith("Nah... You feel too relaxed to do that.", failedMovement);
    addStartsWith("Maybe you should get on your feet first?", failedMovement);
    addStartsWith("In your dreams, or what?", failedMovement);
    addStartsWith("If you still want to try, you must 'climb' there.", failedMovement);
    // The door
    addEndsWith("seems to be closed.", failedMovement);
    // The (a|de)scent
    addEndsWith("is too steep, you need to climb to go there.", failedMovement);
    // A pack horse
    addEndsWith("is too exhausted.", failedMovement);
    addRegex(R"(^ZBLAM! .+ doesn't want you riding (him|her|it) anymore.$)", failedMovement);

    auto look = [this](StringView /*view*/) { getQueue().enqueue(CommandEnum::LOOK); };
    addStartsWith("You follow", look);

    auto flee = [this](StringView /*view*/) { getQueue().enqueue(CommandEnum::FLEE); };
    addStartsWith("You flee head", flee);

    auto scout = [this](StringView /*view*/) { getQueue().enqueue(CommandEnum::SCOUT); };
    addStartsWith("You quietly scout", scout);

    /// Time
    addStartsWith("The current time is",
                  [this](StringView view) { m_mumeClock.parseClockTime(view.toQString()); });
    addEndsWith("of the Third Age.",
                [this](StringView view) { m_mumeClock.parseMumeTime(view.toQString()); });

    /// Stat
    addRegex(R"(^)"
             R"((?:Needed:(?: [\d,]+ xp)?(?:,? [\d,]+ tp)\. )?)" // Needed
             R"((Gold|Lauren): [\d,]+\.)"                        // Gold
             R"((?: Iv: [^.]+\.)?)"                              // God Invis Level
             R"( Alert: \w+\.)"                                  // Alertness
             R"((?: Condition: [^.]+\.)?)",                      // Hunger or Thirst
             [this](StringView /*view*/) {
                 const auto list = getTimers().getStatCommandEntry();
                 if (!list.empty()) {
                     sendToUser(SendToUserSource::FromMMapper, list);
                 }
             });
}

// FIXME: always returns false, so just remove the return value,
//  or fix this to return true sometimes.
bool MumeXmlParserBase::evalActionMap(StringView line)
{
    if (line.empty()) {
        return false;
    }

    auto &map = m_actionMap;

    auto range = map.equal_range(line.firstChar());
    for (auto it = range.first; it != range.second; ++it) {
        const std::unique_ptr<IAction> &action = it->second;
        action->match(line);
    }

    range = map.equal_range(0);
    for (auto it = range.first; it != range.second; ++it) {
        const std::unique_ptr<IAction> &action = it->second;
        action->match(line);
    }

    return false;
}
