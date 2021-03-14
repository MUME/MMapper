// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "abstractparser.h"

#include <cassert>
#include <memory>

#include "../clock/mumeclock.h"
#include "../pandoragroup/mmapper2group.h"
#include "Action.h"

void AbstractParser::initActionMap()
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
            if (match.length() > 2 && match[0] == '^' && match[1] != '\\' && match[1] != '(')
                return match[1];
            return 0;
        }();
        map.emplace(hint, std::make_unique<RegexAction>(match, callback));
    };

    /// Positions
    auto sleeping = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterPositionEnum::SLEEPING);
    };
    addStartsWith("You go to sleep.", sleeping);
    addStartsWith("You are already sound asleep.", sleeping);
    auto sitting = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterPositionEnum::SITTING);
    };
    addStartsWith("You wake, and sit up.", sitting);
    addStartsWith("You sit down.", sitting);
    addStartsWith("You stop resting and sit up.", sitting);
    addStartsWith("You're sitting already.", sitting);
    addStartsWith("You feel better and sit up.", sitting); // incap -> sitting
    auto resting = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterPositionEnum::RESTING);
    };
    addStartsWith("You rest your tired bones.", resting);
    addStartsWith("You sit down and rest your tired bones.", resting);
    addStartsWith("You are already resting.", resting);
    auto standing = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterPositionEnum::STANDING);
    };
    addStartsWith("You stop resting and stand up.", standing);
    addStartsWith("You stand up.", standing);
    addStartsWith("You are already standing.", standing);
    auto incap = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterPositionEnum::INCAPACITATED);
    };
    addStartsWith("You are incapacitated and will slowly die, if not aided.", incap);
    addStartsWith("You are in a pretty bad shape, unable to do anything!", incap);
    addStartsWith("You're stunned and will probably die soon if no-one helps you.", incap);
    addStartsWith("You are mortally wounded and will die soon if not aided.", incap);
    auto dead = [this](StringView /*view*/) {
        m_queue.clear();
        pathChanged();
        emit releaseAllPaths();
        markCurrentCommand();
        m_group.sendEvent(CharacterPositionEnum::DEAD);
    };
    addStartsWith("You are dead!", dead);

    /// Bash
    auto bashOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::BASHED, false);
    };
    auto bashOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::BASHED, true);
    };
    addStartsWith("Your head stops stinging.", bashOff);
    // No-bash mob
    addStartsWith("You foolishly hurl yourself at", bashOn);
    // Standard bash skill
    addEndsWith("sends you sprawling with a powerful bash.", bashOn);
    // cave-lion
    addEndsWith("leaps at your throat and sends you sprawling.", bashOn);
    // cave-worm
    addEndsWith("whips its tail around, and sends you sprawling!", bashOn);
    // various trees
    addEndsWith("sends you sprawling.", bashOn);

    /// Blind
    auto blindOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::BLIND, false);
    };
    auto blindOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::BLIND, true);
    };
    // blind spell
    addStartsWith("You have been blinded!", blindOn);
    addStartsWith("You feel a cloak of blindness dissolve.", blindOff);
    // flash powder
    addStartsWith("An extremely bright flash of light stuns you!", blindOn);
    addStartsWith("Your head stops spinning and you can see clearly again.", blindOff);

    /// Bleeding
    auto bleedOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::BLEEDING, false);
    };
    auto bleedOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::BLEEDING, true);
    };
    addStartsWith("You bleed from open wounds.", bleedOn);
    addRegex(
        R"(^- a (deep|serious|grievous|critical) wound at the .+ \((clean|dirty|dirty, suppurating)\))",
        bleedOn);
    // Cure critic
    addStartsWith("You can feel the broken bones within you heal and reshape themselves.", bleedOff);

    /// Slept
    auto sleptOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::SLEPT, false);
    };
    auto sleptOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::SLEPT, true);
        m_group.sendEvent(CharacterPositionEnum::SLEEPING);
    };
    addStartsWith("You feel very sleepy... zzzzzz", sleptOn);
    addStartsWith("You feel less tired.", sleptOff);

    /// Poisoned
    auto poisonOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::POISONED, false);
    };
    auto poisonOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::POISONED, true);
    };
    // REVISIT: Is this a disease message?
    addStartsWith("You feel very sick.", poisonOn);
    // Arachnia
    addStartsWith("Your body turns numb as the poison speeds to your brain!", poisonOn);
    addStartsWith("You feel sleepy.", poisonOn);
    addStartsWith("Your limbs are becoming cold and heavy, your eyelids close.", poisonOn);
    // Generic poison
    addStartsWith("You feel bad.", poisonOn);
    addStartsWith("- poison (type: ", poisonOn);
    // Psylonia
    addStartsWith("You suddenly feel a terrible headache!", poisonOn);
    addStartsWith("A hot flush overwhelms your brain and makes you dizzy.", poisonOn);
    // Venom
    addStartsWith("The venom enters your body!", poisonOn);
    addStartsWith("The venom runs into your veins!", poisonOn);
    // Remove poison
    addStartsWith("A warm feeling runs through your body, you feel better.", poisonOff);
    addStartsWith("A strange feeling runs through your body.", poisonOff);
    // Heal
    addStartsWith("A warm feeling fills your body.", poisonOff);
    // Antidote
    addStartsWith("- antidote", poisonOff);
    addStartsWith("A warm feeling runs through your body.", poisonOff);
    addStartsWith("You feel a strange taste in your mouth.", poisonOff);

    /// Hungry
    auto hungerOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::HUNGRY, false);
    };
    auto hungerOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::HUNGRY, true);
    };
    addStartsWith("You begin to feel hungry.", hungerOn);
    addStartsWith("You are hungry.", hungerOn);
    addStartsWith("You are full.", hungerOff);
    addStartsWith("You are too full to eat more!", hungerOff);

    /// Thirsty
    auto thirstOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::THIRSTY, false);
    };
    auto thirstOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::THIRSTY, true);
    };
    addStartsWith("You begin to feel thirsty.", thirstOn);
    addStartsWith("You are thirsty.", thirstOn);
    addStartsWith("You do not feel thirsty anymore.", thirstOff);
    addStartsWith("Your stomach can't contain anymore!", thirstOff);
    // create water
    addStartsWith("You feel less thirsty.", thirstOff);
    addStartsWith("You are not thirsty anymore.", thirstOff);

    /// Path Machine: Prespam
    auto failedMovement = [this](StringView /*view*/) {
        if (!m_queue.isEmpty()) {
            MAYBE_UNUSED const auto ignored = //
                m_queue.dequeue();
        }
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
    auto zblam = [this](StringView /*view*/) {
        if (!m_queue.isEmpty()) {
            MAYBE_UNUSED const auto ignored = //
                m_queue.dequeue();
        }
        pathChanged();
        m_group.sendEvent(CharacterPositionEnum::RESTING);
    };
    addRegex(R"(^ZBLAM! .+ doesn't want you riding (him|her|it) anymore.$)", zblam);

    auto look = [this](StringView /*view*/) { m_queue.enqueue(CommandEnum::LOOK); };
    addStartsWith("You flee head", look);
    addStartsWith("You follow", look);

    auto scout = [this](StringView /*view*/) { m_queue.enqueue(CommandEnum::SCOUT); };
    addStartsWith("You quietly scout", scout);

    auto river = [this](StringView /*view*/) {
        if (m_move != CommandEnum::NONE)
            return;

        // Predict our movement given what we know about flow flags
        auto rs = RoomSelection(*m_mapData);
        if (const Room *const r = rs.getRoom(getNextPosition())) {
            for (auto i : ALL_EXITS_NESWUD) {
                const Exit &e = r->exit(i);
                const ExitFlags ef = e.getExitFlags();
                if (!ef.isExit())
                    continue;
                if (ef.isFlow()) {
                    // Override movement with the flow direction
                    m_move = getCommand(i);
                    return;
                }
            }
        }
    };
    addStartsWith("You are swept away by the current.", river);       // Anduin River
    addStartsWith("You are borne along by a strong current.", river); // General River

    /// Time
    addStartsWith("The current time is",
                  [this](StringView view) { m_mumeClock->parseClockTime(view.toQString()); });
    addEndsWith("of the Third Age.",
                [this](StringView view) { m_mumeClock->parseMumeTime(view.toQString()); });

    /// Score
    addRegex(R"(^(You have )?\d+/\d+ hits?(, \d+/\d+ mana,)? and \d+/\d+ move(ment point)?s.$)",
             [this](StringView view) { emit sendScoreLineEvent(view.toQByteArray()); });

    /// Search, reveal, and flush
    addStartsWith("You begin to search...", [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::SEARCH, true);
    });

    /// Snared
    auto snaredOff = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::SNARED, false);
    };
    auto snaredOn = [this](StringView /*view*/) {
        m_group.sendEvent(CharacterAffectEnum::SNARED, true);
    };
    // water serpent
    addEndsWith("wraps itself tightly around you and begins squeezing.", snaredOn);
    addStartsWith("You can't escape the constricting coils!", snaredOn);
    addStartsWith("You managed to break free!", snaredOff);

    // black roots, roots
    addRegex(R"(^You are trapped by some (black )?roots.$)", snaredOn);
    addStartsWith("You can't seem to escape the roots!", snaredOn);
    addStartsWith("With effort you break free from the grasp of the roots.", snaredOff);

    // rotten tree
    addStartsWith("You get entangled in numerous branches and twisting roots of a rotten tree.",
                  snaredOn);
    addStartsWith("You can't seem to escape the wet, rotten shrubbery!", snaredOn);
    addStartsWith("You break free from the wet grasp of a rotten tree.", snaredOff);

    // bindweed
    addStartsWith("You are tangled in the bindweed and cannot break free.", snaredOn);
    addStartsWith("You manage to pull away from the grasp of the bindweed.", snaredOff);

    // smothering mass of vines
    addStartsWith("You are entangled by a smothering mass of vines.", snaredOn);
    addStartsWith("You can't seem to escape the vines!", snaredOn);
    addStartsWith("You manage to break free of the constricting vines.", snaredOff);

    // small tangle of roots
    addStartsWith("Your feet have been snared by a tangle of roots.", snaredOn);
    addStartsWith("You can't seem to escape the tangled roots!", snaredOn);
    addStartsWith("With effort you break free from the grasp of the tangled roots.", snaredOff);

    // Unqalome shackles
    addStartsWith("A pair of shackles reach you and wrap tightly around your wrists!", snaredOn);
    addStartsWith("The shackles won't let you move that easily!", snaredOn);
    addStartsWith("You manage to shake off the shackles.", snaredOff);

    // BM spiders
    addRegex(R"(^A great, dark spider.+quickly begins to weave a tight, sticky web around your body!$)",
             snaredOn);
    addRegex(R"(^A great, dark spider.+burdens your movement wrapping you layer after layer!$)",
             snaredOn);
    addRegex(
        R"(^You stagger for your balance as a great, dark spider.+wraps the sticky cords faster and faster around you!$)",
        snaredOn);
    addStartsWith("The sticky cords that surround you prevent you from doing that.", snaredOn);
    addStartsWith("The sticky webs pull you back!", snaredOn);
    addStartsWith("You have been cocooned.", snaredOn);
    addStartsWith("You can't get yourself to relax while bound in such an awkward position.",
                  snaredOn);
    addStartsWith("After some struggle you remove the last web on you.", snaredOff);
    addStartsWith("Someone freed you while you were away.", snaredOff);

    // Kraken tentancle
    addRegex(R"(^A tentacle.+entangles you!$)", snaredOn);
    addStartsWith("You can't free your hands to cast any spells at all!", snaredOn);
    addStartsWith("You are pulled back by a tentacle", snaredOn);
    addRegex(R"(^You manage to break free of a tentacle's.+hold.$)", snaredOff);
}

bool AbstractParser::evalActionMap(StringView line)
{
    if (line.empty())
        return false;

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
