#pragma once
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

#ifndef MUMEMOMENT_H
#define MUMEMOMENT_H

#include <cstdint>
#include <QString>

static constexpr const int MUME_START_YEAR = 2850;

enum class MumeTime { TIME_UNKNOWN = 0, TIME_DAWN, TIME_DAY, TIME_DUSK, TIME_NIGHT };
enum class MumeSeason {
    SEASON_UNKNOWN = 0,
    SEASON_WINTER,
    SEASON_SPRING,
    SEASON_SUMMER,
    SEASON_AUTUMN
};

enum class MumeMoonPhase {
    PHASE_UNKNOWN = -1,    // Illumination Levels
    PHASE_WAXING_CRESCENT, //  1,  2,  3
    PHASE_FIRST_QUARTER,   //  4,  5,  6
    PHASE_WAXING_GIBBOUS,  //  7,  8,  9
    PHASE_FULL_MOON,       // 10, 11, 12
    PHASE_WANING_GIBBOUS,  // 11, 10,  9
    PHASE_THIRD_QUARTER,   //  8,  7,  6
    PHASE_WANING_CRESCENT, //  5,  4,  3
    PHASE_NEW_MOON         //  2,  1,  0
};
enum class MumeMoonVisibility {
    MOON_POSITION_UNKNOWN = -1,
    MOON_HIDDEN,
    MOON_RISE,
    MOON_VISIBLE,
    MOON_SET
};

class MumeMoment
{
public:
    explicit MumeMoment(int year, int month, int day, int hour, int minute);
    static MumeMoment sinceMumeEpoch(int64_t secsSinceMumeStartEpoch);

    int dayOfYear() const;
    int weekDay() const;
    int toSeconds() const;
    MumeSeason toSeason() const;
    MumeTime toTimeOfDay() const;

    int moonCycle() const;
    int moonRise() const;
    int moonSet() const;
    int moonPosition() const;
    int moonLevel() const;
    MumeMoonPhase toMoonPhase() const;
    MumeMoonVisibility toMoonVisibility() const;
    QString toMumeMoonTime() const;
    QString toMoonCountDown() const;

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
};

#endif // MUMEMOMENT_H
