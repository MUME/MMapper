/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/
#include "mumemoment.h"

#include <array>
#include <iostream>

#include "mumeclock.h"

static constexpr const int MUME_MINUTES_PER_HOUR = 60;
static constexpr const int MUME_HOURS_PER_DAY = 24;
static constexpr const int MUME_DAYS_PER_MONTH = 30;
static constexpr const int MUME_MONTHS_PER_YEAR = 12;

static constexpr const int MUME_MINUTES_PER_DAY = MUME_HOURS_PER_DAY * MUME_MINUTES_PER_HOUR;
static constexpr const int MUME_MINUTES_PER_MONTH = MUME_DAYS_PER_MONTH * MUME_MINUTES_PER_DAY;
static constexpr const int MUME_MINUTES_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_MINUTES_PER_MONTH;

static constexpr const int MUME_DAYS_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_DAYS_PER_MONTH;
static_assert(MUME_DAYS_PER_YEAR == 360, "");

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
