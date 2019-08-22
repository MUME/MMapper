#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    bool isMoonVisible() const { return toMoonVisibility() > MumeMoonVisibility::MOON_HIDDEN; }
    bool isMoonBright() const { return moonLevel() > 4; }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
};
