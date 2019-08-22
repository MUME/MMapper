// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mumemoment.h"

#include <array>
#include <iostream>

#include "mumeclock.h"

static constexpr const int MUME_MINUTES_PER_HOUR = 60;
static constexpr const int MUME_HOURS_PER_DAY = 24;
static constexpr const int MUME_DAYS_PER_MONTH = 30;
static constexpr const int MUME_MONTHS_PER_YEAR = 12;

static constexpr const int MUME_DAYS_PER_MOON_CYCLE = 24;
static constexpr const int MUME_HOURS_PER_MOON_POSITION = 24;

static constexpr const int MUME_MINUTES_PER_DAY = MUME_HOURS_PER_DAY * MUME_MINUTES_PER_HOUR;
static constexpr const int MUME_MINUTES_PER_MONTH = MUME_DAYS_PER_MONTH * MUME_MINUTES_PER_DAY;
static constexpr const int MUME_MINUTES_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_MINUTES_PER_MONTH;

static constexpr const int MUME_DAYS_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_DAYS_PER_MONTH;
static_assert(MUME_DAYS_PER_YEAR == 360);

static void maybe_warn_if_not_clamped(
    const char *const name, bool &warned, const int val, const int lo, const int hi)
{
    if (warned)
        return;

    if (val >= lo && val < hi)
        return;

#ifndef NDEBUG
    std::cerr << "[at " << __FILE__ << ":" << __LINE__ << "] ";
#endif
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
    static bool year_warned = false, month_warned = false, day_warned = false, hour_warned = false,
                minute_warned = false;

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

int MumeMoment::toSeconds() const
{
    return minute + hour * MUME_MINUTES_PER_HOUR + day * MUME_MINUTES_PER_DAY
           + month * MUME_MINUTES_PER_MONTH + (year - MUME_START_YEAR) * MUME_MINUTES_PER_YEAR;
}

MumeSeason MumeMoment::toSeason() const
{
    using WMN = typename MumeClock::WestronMonthNames;
    switch (static_cast<WMN>(month)) {
    case WMN::Afteryule:
    case WMN::Solmath:
    case WMN::Rethe:
        return MumeSeason::SEASON_WINTER;
    case WMN::Astron:
    case WMN::Thrimidge:
    case WMN::Forelithe:
        return MumeSeason::SEASON_SPRING;
    case WMN::Afterlithe:
    case WMN::Wedmath:
    case WMN::Halimath:
        return MumeSeason::SEASON_SUMMER;
    case WMN::Winterfilth:
    case WMN::Blotmath:
    case WMN::Foreyule:
        return MumeSeason::SEASON_AUTUMN;

    case MumeClock::WestronMonthNames::UnknownWestronMonth:
        break;
    };
    return MumeSeason::SEASON_UNKNOWN;
}

MumeTime MumeMoment::toTimeOfDay() const
{
    const auto dawnDusk = MumeClock::getDawnDusk(month);
    const int dawn = dawnDusk.dawnHour;
    const int dusk = dawnDusk.duskHour;
    if (hour == dawn) {
        return MumeTime::TIME_DAWN;
    } else if (hour == dusk) {
        return MumeTime::TIME_DUSK;
    } else if (hour < dawn || hour > dusk) {
        return MumeTime::TIME_NIGHT;
    }
    return MumeTime::TIME_DAY;
}

int MumeMoment::moonCycle() const
{
    // Moon cycle is 24 days long and starts with the ending of the new moon
    return (year * MUME_DAYS_PER_YEAR + month * MUME_DAYS_PER_MONTH + day)
           % MUME_DAYS_PER_MOON_CYCLE;
}

int MumeMoment::moonRise() const
{
    return (moonCycle() + 6) % MUME_DAYS_PER_MOON_CYCLE;
}

int MumeMoment::moonSet() const
{
    return (moonCycle() + 18) % MUME_DAYS_PER_MOON_CYCLE;
}

int MumeMoment::moonPosition() const
{
    // Position of moon in the sky out of 24
    // Below horizon is position >= 12
    return (hour - moonRise() + MUME_HOURS_PER_MOON_POSITION) % MUME_HOURS_PER_MOON_POSITION;
}

int MumeMoment::moonLevel() const
{
    // Illumination level seems to be between 0-12
    // Levels > 4 make the moon show up in the prompt as ')'
    const int dayOfCycle = moonCycle();
    if (dayOfCycle <= 12)
        return dayOfCycle; // Increasing illumination
    else
        return MUME_DAYS_PER_MOON_CYCLE - dayOfCycle; // Decreasing illumination
}

MumeMoonPhase MumeMoment::toMoonPhase() const
{
    const int dayOfCycle = moonCycle();
    if (dayOfCycle <= 2)
        return MumeMoonPhase::PHASE_NEW_MOON;
    else if (dayOfCycle >= 3 && dayOfCycle <= 5)
        return MumeMoonPhase::PHASE_WAXING_CRESCENT;
    else if (dayOfCycle >= 6 && dayOfCycle <= 8)
        return MumeMoonPhase::PHASE_FIRST_QUARTER;
    else if (dayOfCycle >= 9 && dayOfCycle <= 11)
        return MumeMoonPhase::PHASE_WAXING_GIBBOUS;
    else if (dayOfCycle == 12)
        return MumeMoonPhase::PHASE_FULL_MOON;
    else if (dayOfCycle >= 13 && dayOfCycle <= 15)
        return MumeMoonPhase::PHASE_WANING_GIBBOUS;
    else if (dayOfCycle >= 16 && dayOfCycle <= 18)
        return MumeMoonPhase::PHASE_THIRD_QUARTER;
    else if (dayOfCycle >= 19 && dayOfCycle <= 21)
        return MumeMoonPhase::PHASE_WANING_CRESCENT;
    else
        return MumeMoonPhase::PHASE_NEW_MOON;
}

MumeMoonVisibility MumeMoment::toMoonVisibility() const
{
    // Moon is not visible because of its position in the sky
    if (moonPosition() >= 12)
        return MumeMoonVisibility::MOON_HIDDEN;

    if (hour == moonRise())
        return MumeMoonVisibility::MOON_RISE;
    else if (hour == moonSet())
        return MumeMoonVisibility::MOON_SET;
    else
        return MumeMoonVisibility::MOON_VISIBLE;
}

QString MumeMoment::toMumeMoonTime() const
{
    const int pos = moonPosition();
    QString visibility = pos < 12 ? "can" : "can not";

    QString phase = "Moon";
    switch (toMoonPhase()) {
    case MumeMoonPhase::PHASE_WAXING_CRESCENT:
    case MumeMoonPhase::PHASE_WANING_CRESCENT:
        phase = "Quarter Moon";
        break;
    case MumeMoonPhase::PHASE_FIRST_QUARTER:
    case MumeMoonPhase::PHASE_THIRD_QUARTER:
        phase = "Half Moon";
        break;
    case MumeMoonPhase::PHASE_WAXING_GIBBOUS:
    case MumeMoonPhase::PHASE_WANING_GIBBOUS:
        phase = "Three-Quarter Moon";
        break;
    case MumeMoonPhase::PHASE_FULL_MOON:
        phase = "Full Moon";
        break;
    case MumeMoonPhase::PHASE_NEW_MOON:
        phase = "New Moon";
        break;
    case MumeMoonPhase::PHASE_UNKNOWN:
    default:
        break;
    }
    phase.append((moonCycle() < 12) ? " (waxing)" : " (waning)");

    QString positionInSky = "sky";
    if (pos < 12) {
        if (pos <= 3) {
            positionInSky = "eastern part of the sky";
        } else if (pos >= 8) {
            positionInSky = "western part of the sky";
        } else {
            positionInSky = "southern part of the sky";
        }
    }
    return QString("You %1 see a %2 in the %3.").arg(visibility).arg(phase).arg(positionInSky);
}

QString MumeMoment::toMoonCountDown() const
{
    int secondsToCountdown = 60 - minute;
    const int pos = moonPosition();
    if (pos < 12) {
        // Moon visible
        secondsToCountdown += (12 - pos - 1) * 60;
    } else {
        secondsToCountdown += (24 - pos - 1) * 60;
    }
    return QString("%1:%2")
        .arg(secondsToCountdown / 60)
        .arg(QString().sprintf("%02d", secondsToCountdown % 60));
}
