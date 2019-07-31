#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>
#include <QString>

static constexpr const int MUME_START_YEAR = 2850;

enum class MumeTimeEnum { UNKNOWN = 0, DAWN, DAY, DUSK, NIGHT };
enum class MumeSeasonEnum { UNKNOWN = 0, WINTER, SPRING, SUMMER, AUTUMN };

enum class MumeMoonPhaseEnum {
    UNKNOWN = -1,    // Illumination Levels
    WAXING_CRESCENT, //  1,  2,  3
    FIRST_QUARTER,   //  4,  5,  6
    WAXING_GIBBOUS,  //  7,  8,  9
    FULL_MOON,       // 10, 11, 12
    WANING_GIBBOUS,  // 11, 10,  9
    THIRD_QUARTER,   //  8,  7,  6
    WANING_CRESCENT, //  5,  4,  3
    NEW_MOON         //  2,  1,  0
};
enum class MumeMoonVisibilityEnum { POSITION_UNKNOWN = -1, HIDDEN, RISE, VISIBLE, SET };

class MumeMoment
{
public:
    explicit MumeMoment(int year, int month, int day, int hour, int minute);
    static MumeMoment sinceMumeEpoch(int64_t secsSinceMumeStartEpoch);

    int dayOfYear() const;
    int weekDay() const;
    int toSeconds() const;
    MumeSeasonEnum toSeason() const;
    MumeTimeEnum toTimeOfDay() const;

    int moonCycle() const;
    int moonRise() const;
    int moonSet() const;
    int moonPosition() const;
    int moonLevel() const;
    MumeMoonPhaseEnum toMoonPhase() const;
    MumeMoonVisibilityEnum toMoonVisibility() const;
    QString toMumeMoonTime() const;
    QString toMoonCountDown() const;
    bool isMoonVisible() const { return toMoonVisibility() > MumeMoonVisibilityEnum::HIDDEN; }
    bool isMoonBright() const { return moonLevel() > 4; }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
};
