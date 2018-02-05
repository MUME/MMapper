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

#define MUME_MINUTES_PER_HOUR 60
#define MUME_HOURS_PER_DAY 24
#define MUME_DAYS_PER_MONTH 30
#define MUME_MONTHS_PER_YEAR 12

#define MUME_MINUTES_PER_DAY     (MUME_HOURS_PER_DAY * MUME_MINUTES_PER_HOUR)
#define MUME_MINUTES_PER_MONTH   (MUME_DAYS_PER_MONTH * MUME_MINUTES_PER_DAY)
#define MUME_MINUTES_PER_YEAR    (MUME_MONTHS_PER_YEAR * MUME_MINUTES_PER_MONTH)

MumeMoment::MumeMoment(int year, int month, int day, int hour, int minute)
{
    m_year = year;
    m_month = month;
    m_day = day;
    m_hour = hour;
    m_minute = minute;
}

MumeMoment::MumeMoment(int secsSinceMumeStartEpoch)
{
    // https://github.com/iheartdisraptor/mume/blob/master/mudlet/scrolls/Clock/lua/clock.lua
    int mumeTimeYears = (secsSinceMumeStartEpoch / MUME_MINUTES_PER_YEAR);
    m_year = MUME_START_YEAR + mumeTimeYears;

    int mumeTimeMinusYears = secsSinceMumeStartEpoch - mumeTimeYears * MUME_MINUTES_PER_YEAR;
    m_month = mumeTimeMinusYears / MUME_MINUTES_PER_MONTH;

    int mumeTimeMinusYearsAndMonths = mumeTimeMinusYears - m_month * MUME_MINUTES_PER_MONTH;
    m_day = mumeTimeMinusYearsAndMonths / MUME_MINUTES_PER_DAY;

    int mumeTimeMinusYearsMonthsAndDays = mumeTimeMinusYearsAndMonths - m_day * MUME_MINUTES_PER_DAY;
    m_hour = mumeTimeMinusYearsMonthsAndDays / MUME_MINUTES_PER_HOUR;

    int mumeTimeMinusYearsMonthDaysAndMinutes = mumeTimeMinusYearsMonthsAndDays - m_hour *
                                                MUME_MINUTES_PER_HOUR;
    m_minute = mumeTimeMinusYearsMonthsAndDays <= 0 ? 0 : mumeTimeMinusYearsMonthDaysAndMinutes;
}

int MumeMoment::toSeconds()
{
    return m_minute
           + m_hour * MUME_MINUTES_PER_HOUR
           + m_day * MUME_MINUTES_PER_DAY
           + m_month * MUME_MINUTES_PER_MONTH
           + (m_year - MUME_START_YEAR) * MUME_MINUTES_PER_YEAR;
}

MumeSeason MumeMoment::toSeason()
{
    switch (m_month) {
    case MumeClock::Afteryule:
    case MumeClock::Solmath:
    case MumeClock::Rethe:
        return SEASON_WINTER;
    case MumeClock::Astron:
    case MumeClock::Thrimidge:
    case MumeClock::Forelithe:
        return SEASON_SPRING;
    case MumeClock::Afterlithe:
    case MumeClock::Wedmath:
    case MumeClock::Halimath:
        return SEASON_SUMMER;
    case MumeClock::Winterfilth:
    case MumeClock::Blotmath:
    case MumeClock::Foreyule:
        return SEASON_AUTUMN;
    default:
        return SEASON_UNKNOWN;
    };
}

MumeTime MumeMoment::toTimeOfDay()
{
    int dawn = MumeClock::s_dawnHour[m_month];
    int dusk = MumeClock::s_duskHour[m_month];
    if (m_hour == dawn)
        return TIME_DAWN;
    else if (m_hour == dusk)
        return TIME_DUSK;
    else if (m_hour < dawn || m_hour > dusk)
        return TIME_NIGHT;
    else
        return TIME_DAY;
}
