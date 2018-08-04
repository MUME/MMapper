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
#include "mumeclock.h"
#include <array>

static constexpr const int MUME_MINUTES_PER_HOUR = 60;
static constexpr const int MUME_HOURS_PER_DAY = 24;
static constexpr const int MUME_DAYS_PER_MONTH = 30;
static constexpr const int MUME_MONTHS_PER_YEAR = 12;

static constexpr const int MUME_MINUTES_PER_DAY = MUME_HOURS_PER_DAY * MUME_MINUTES_PER_HOUR;
static constexpr const int MUME_MINUTES_PER_MONTH = MUME_DAYS_PER_MONTH * MUME_MINUTES_PER_DAY;
static constexpr const int MUME_MINUTES_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_MINUTES_PER_MONTH;

MumeMoment::MumeMoment(int year, int month, int day, int hour, int minute)
    : m_year{year}
    , m_month{month}
    , m_day{day}
    , m_hour{hour}
    , m_minute{minute}
{}

MumeMoment::MumeMoment(const int secsSinceMumeStartEpoch)
{
    // https://github.com/iheartdisraptor/mume/blob/master/mudlet/scrolls/Clock/lua/clock.lua
    const int mumeTimeYears = (secsSinceMumeStartEpoch / MUME_MINUTES_PER_YEAR);
    m_year = MUME_START_YEAR + mumeTimeYears;

    const int mumeTimeMinusYears = secsSinceMumeStartEpoch - mumeTimeYears * MUME_MINUTES_PER_YEAR;
    m_month = mumeTimeMinusYears / MUME_MINUTES_PER_MONTH;

    const int mumeTimeMinusYearsAndMonths = mumeTimeMinusYears - m_month * MUME_MINUTES_PER_MONTH;
    m_day = mumeTimeMinusYearsAndMonths / MUME_MINUTES_PER_DAY;

    const int mumeTimeMinusYearsMonthsAndDays = mumeTimeMinusYearsAndMonths
                                                - m_day * MUME_MINUTES_PER_DAY;
    m_hour = mumeTimeMinusYearsMonthsAndDays / MUME_MINUTES_PER_HOUR;

    const int mumeTimeMinusYearsMonthDaysAndMinutes = mumeTimeMinusYearsMonthsAndDays
                                                      - m_hour * MUME_MINUTES_PER_HOUR;
    m_minute = (mumeTimeMinusYearsMonthsAndDays <= 0) ? 0 : mumeTimeMinusYearsMonthDaysAndMinutes;
}

int MumeMoment::toSeconds() const
{
    return m_minute + m_hour * MUME_MINUTES_PER_HOUR + m_day * MUME_MINUTES_PER_DAY
           + m_month * MUME_MINUTES_PER_MONTH + (m_year - MUME_START_YEAR) * MUME_MINUTES_PER_YEAR;
}

MumeSeason MumeMoment::toSeason() const
{
    using WMN = typename MumeClock::WestronMonthNames;
    switch (static_cast<WMN>(m_month)) {
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
    default:
        return MumeSeason::SEASON_UNKNOWN;
    };
}

MumeTime MumeMoment::toTimeOfDay() const
{
    const int dawn = MumeClock::s_dawnHour[m_month];
    const int dusk = MumeClock::s_duskHour[m_month];
    if (m_hour == dawn) {
        return MumeTime::TIME_DAWN;
    } else if (m_hour == dusk) {
        return MumeTime::TIME_DUSK;
    } else if (m_hour < dawn || m_hour > dusk) {
        return MumeTime::TIME_NIGHT;
    }
    return MumeTime::TIME_DAY;
}
