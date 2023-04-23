// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumemoment.h"

#include <iostream>

#include "../global/Debug.h"
#include "mumeclock.h"

static void maybe_warn_if_not_clamped(
    const char *const name, bool &warned, const int val, const int lo, const int hi)
{
    if (warned)
        return;

    if (val >= lo && val < hi)
        return;

    if constexpr (IS_DEBUG_BUILD) {
        std::cerr << "[at " << __FILE__ << ":" << __LINE__ << "] ";
    }
    std::cerr << "WARNING: soft assertion failure: " << name << "(" << val
              << ") is not in the half-open interval '[" << lo << ".." << hi << ")'" << std::endl;
    warned = true;
}

MumeMoment::MumeMoment(
    const int year, const int month, const int day, const int hour, const int minute)
    : year{year}
    , month{month}
    , day{day}
    , hour{hour}
    , minute{minute}
{
    static thread_local bool year_warned = false, month_warned = false, day_warned = false,
                             hour_warned = false, minute_warned = false;

#define soft_assert_clamped(x, lo, hi) maybe_warn_if_not_clamped(#x, x##_warned, (x), (lo), (hi))
    soft_assert_clamped(year, 2100, 4100);
    soft_assert_clamped(month, 0, MUME_MONTHS_PER_YEAR);
    soft_assert_clamped(day, 0, MUME_DAYS_PER_MONTH);
    soft_assert_clamped(hour, 0, MUME_HOURS_PER_DAY);
    soft_assert_clamped(minute, 0, MUME_MINUTES_PER_HOUR);
#undef soft_assert_clamped
}

MumeMoment MumeMoment::sinceMumeEpoch(const int64_t secsSinceMumeStartEpoch)
{
    // https://github.com/iheartdisraptor/mume/blob/master/mudlet/scrolls/Clock/lua/clock.lua
    const int mumeTimeYears = static_cast<int>(secsSinceMumeStartEpoch / MUME_MINUTES_PER_YEAR);
    const int year = MUME_START_YEAR + mumeTimeYears;

    const int mumeTimeMinusYears = static_cast<int>(secsSinceMumeStartEpoch
                                                    - mumeTimeYears * MUME_MINUTES_PER_YEAR);
    const int month = mumeTimeMinusYears / MUME_MINUTES_PER_MONTH;

    const int mumeTimeMinusYearsAndMonths = mumeTimeMinusYears - month * MUME_MINUTES_PER_MONTH;
    const int day = mumeTimeMinusYearsAndMonths / MUME_MINUTES_PER_DAY;

    const int mumeTimeMinusYearsMonthsAndDays = mumeTimeMinusYearsAndMonths
                                                - day * MUME_MINUTES_PER_DAY;
    const int hour = mumeTimeMinusYearsMonthsAndDays / MUME_MINUTES_PER_HOUR;

    const int mumeTimeMinusYearsMonthDaysAndMinutes = mumeTimeMinusYearsMonthsAndDays
                                                      - hour * MUME_MINUTES_PER_HOUR;
    const int minute = (mumeTimeMinusYearsMonthsAndDays <= 0)
                           ? 0
                           : mumeTimeMinusYearsMonthDaysAndMinutes;

    return MumeMoment{year, month, day, hour, minute};
}

int MumeMoment::dayOfYear() const
{
    return month * MUME_DAYS_PER_MONTH + day;
}

int MumeMoment::weekDay() const
{
    return dayOfYear() % 7;
}

int64_t MumeMoment::toSeconds() const
{
    return minute + hour * MUME_MINUTES_PER_HOUR + day * MUME_MINUTES_PER_DAY
           + month * MUME_MINUTES_PER_MONTH + (year - MUME_START_YEAR) * MUME_MINUTES_PER_YEAR;
}

MumeSeasonEnum MumeMoment::toSeason() const
{
    using WMN = typename MumeClock::WestronMonthNamesEnum;
    switch (static_cast<WMN>(month)) {
    case WMN::Afteryule:
    case WMN::Solmath:
    case WMN::Rethe:
        return MumeSeasonEnum::WINTER;
    case WMN::Astron:
    case WMN::Thrimidge:
    case WMN::Forelithe:
        return MumeSeasonEnum::SPRING;
    case WMN::Afterlithe:
    case WMN::Wedmath:
    case WMN::Halimath:
        return MumeSeasonEnum::SUMMER;
    case WMN::Winterfilth:
    case WMN::Blotmath:
    case WMN::Foreyule:
        return MumeSeasonEnum::AUTUMN;

    case MumeClock::WestronMonthNamesEnum::UnknownWestronMonth:
        break;
    }
    return MumeSeasonEnum::UNKNOWN;
}

MumeTimeEnum MumeMoment::toTimeOfDay() const
{
    const auto dawnDusk = MumeClock::getDawnDusk(month);
    const int dawn = dawnDusk.dawnHour;
    const int dusk = dawnDusk.duskHour;
    if (hour == dawn) {
        return MumeTimeEnum::DAWN;
    } else if (hour == dusk) {
        return MumeTimeEnum::DUSK;
    } else if (hour < dawn || hour > dusk) {
        return MumeTimeEnum::NIGHT;
    }
    return MumeTimeEnum::DAY;
}

int MumeMoment::moonZenithMinutes() const
{
    // At what minute of day the moon is at its highest point in the sky
    const auto count = toSeconds() * MUME_MINUTES_PER_DAY;
    return static_cast<int>((count / MUME_MINUTES_PER_MOON_CYCLE) % MUME_MINUTES_PER_DAY);
}

MumeMoonPositionEnum MumeMoment::moonPosition() const
{
    // Position of moon in the sky
    int rise = moonZenithMinutes() + MUME_MINUTES_PER_DAY * 3 / 4;
    if (rise >= MUME_MINUTES_PER_DAY)
        rise -= MUME_MINUTES_PER_DAY;
    int now = minute + hour * MUME_MINUTES_PER_HOUR;
    if (now < rise)
        now += MUME_MINUTES_PER_DAY;
    /* dir 0-15: 0: east, 1/2: southeast, 3/4: south, 5/6: southwest; 7: west;
           8-15: under the horizon*/
    int dir = (now - rise) * 16 / MUME_MINUTES_PER_DAY;
    if (dir < 0 || dir > 15)
        qWarning() << "Moon position dir is out of whack" << dir;
    if (dir < 8) {
        static const constexpr int EAST = static_cast<int>(MumeMoonPositionEnum::EAST);
        static const constexpr int MIN = static_cast<int>(MumeMoonPositionEnum::INVISIBLE);
        static const constexpr int MAX = static_cast<int>(MumeMoonPositionEnum::WEST);
        return static_cast<MumeMoonPositionEnum>(std::clamp(EAST + (dir + 1) / 2, MIN, MAX));
    }
    return MumeMoonPositionEnum::INVISIBLE;
}

int MumeMoment::moonLevel() const
{
    // Illumination level seems to be between 0-12
    // 0 is new moon, 12 is full moon
    // Levels > 4 make the moon show up in the prompt as ')'
    int level = (moonZenithMinutes() + MUME_MINUTES_PER_HOUR / 2) / MUME_MINUTES_PER_HOUR;
    return abs(12 - level);
}

MumeMoonPhaseEnum MumeMoment::moonPhase() const
{
    // Moon starts off as full phase at the start epoch
    // Divide level by three for phase (new, 1/4, 1/2, 3/4, full)
    const int phase = std::clamp(moonLevel() / 3, 0, 4);
    static const constexpr MumeMoonPhaseEnum WAXING[] = {MumeMoonPhaseEnum::NEW_MOON,
                                                         MumeMoonPhaseEnum::WAXING_CRESCENT,
                                                         MumeMoonPhaseEnum::FIRST_QUARTER,
                                                         MumeMoonPhaseEnum::WAXING_GIBBOUS,
                                                         MumeMoonPhaseEnum::FULL_MOON};
    static const constexpr MumeMoonPhaseEnum WANING[] = {MumeMoonPhaseEnum::NEW_MOON,
                                                         MumeMoonPhaseEnum::WANING_CRESCENT,
                                                         MumeMoonPhaseEnum::THIRD_QUARTER,
                                                         MumeMoonPhaseEnum::WANING_GIBBOUS,
                                                         MumeMoonPhaseEnum::FULL_MOON};
    return isMoonWaxing() ? WAXING[phase] : WANING[phase];
}

MumeMoonVisibilityEnum MumeMoment::moonVisibility() const
{
    if (!isMoonVisible())
        return MumeMoonVisibilityEnum::BELOW_HORIZON;

    return (isMoonBright() && toTimeOfDay() >= MumeTimeEnum::DUSK) ? MumeMoonVisibilityEnum::BRIGHT
                                                                   : MumeMoonVisibilityEnum::DIM;
}

QString MumeMoment::toMumeMoonTime() const
{
    const int phase = std::clamp(moonLevel() / 3, 0, 4);
    static const constexpr char *const PHASE_MESSAGES[] = {"new",
                                                           "quarter",
                                                           "half",
                                                           "three-quarter",
                                                           "full"};

    auto positionInSky = "";
    switch (moonPosition()) {
    case MumeMoonPositionEnum::UNKNOWN:
        break;
    case MumeMoonPositionEnum::INVISIBLE:
        positionInSky = "is below the horizon";
        break;
    case MumeMoonPositionEnum::EAST:
        positionInSky = "to the east";
        break;
    case MumeMoonPositionEnum::SOUTHEAST:
        positionInSky = "to the southeast";
        break;
    case MumeMoonPositionEnum::SOUTH:
        positionInSky = "to the south";
        break;
    case MumeMoonPositionEnum::SOUTHWEST:
        positionInSky = "to the southwest";
        break;
    case MumeMoonPositionEnum::WEST:
        positionInSky = "to the west";
        break;
    }
    return QString("%1 %2%3 moon %4.")
        .arg(!isMoonVisible() ? "The"
             : isMoonBright() ? "You can see a"
                              : "You can not see a")
        .arg((phase < 1 || phase > 3 ? ""
              : isMoonWaxing()       ? "waxing "
                                     : "waning "))
        .arg(PHASE_MESSAGES[phase])
        .arg(positionInSky);
}

QString MumeMoment::toMoonVisibilityCountDown() const
{
    int rise = moonZenithMinutes() + MUME_MINUTES_PER_DAY * 3 / 4;
    if (rise >= MUME_MINUTES_PER_DAY)
        rise -= MUME_MINUTES_PER_DAY;
    int now = minute + hour * MUME_MINUTES_PER_HOUR;
    if (now < rise)
        now += MUME_MINUTES_PER_DAY;
    int set = rise + MUME_MINUTES_PER_DAY / 2;

    int secondsToCountdown = 0;
    if (now < rise)
        secondsToCountdown = rise - now;
    else if (now < set)
        secondsToCountdown = set - now;
    else
        secondsToCountdown = MUME_MINUTES_PER_MOON_REVOLUTION - now + rise;

    // TODO: What about when the moon is below level 4?

    return QString("%1:%2")
        .arg(secondsToCountdown / 60)
        .arg(secondsToCountdown % 60, 2, 10, QChar('0'));
}

bool MumeMoment::isMoonWaxing() const
{
    return moonZenithMinutes() >= MUME_MINUTES_PER_DAY / 2;
}
