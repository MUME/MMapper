#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <cstdint>

#include <QString>

static constexpr const int MUME_START_YEAR = 2850;

static constexpr const int MUME_MINUTES_PER_HOUR = 60;
static constexpr const int MUME_HOURS_PER_DAY = 24;
static constexpr const int MUME_DAYS_PER_MONTH = 30;
static constexpr const int MUME_MONTHS_PER_YEAR = 12;

static constexpr const int MUME_MINUTES_PER_DAY = MUME_HOURS_PER_DAY * MUME_MINUTES_PER_HOUR;
static constexpr const int MUME_MINUTES_PER_MONTH = MUME_DAYS_PER_MONTH * MUME_MINUTES_PER_DAY;
static constexpr const int MUME_MINUTES_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_MINUTES_PER_MONTH;

static constexpr const int MUME_DAYS_PER_YEAR = MUME_MONTHS_PER_YEAR * MUME_DAYS_PER_MONTH;
static_assert(MUME_DAYS_PER_YEAR == 360);

// real world synodic month: 29d 12h 44min
static constexpr const int MUME_MINUTES_PER_MOON_CYCLE = ((29 * MUME_HOURS_PER_DAY + 12)
                                                              * MUME_MINUTES_PER_HOUR
                                                          + 44);
// time taken for moon to move to the next phase
static constexpr const int MUME_MINUTES_PER_MOON_PHASE = MUME_MINUTES_PER_MOON_CYCLE / 8;
// time taken to revolve around earth into the same position
static constexpr const int MUME_MINUTES_PER_MOON_REVOLUTION = (MUME_MINUTES_PER_DAY
                                                               * MUME_MINUTES_PER_MOON_CYCLE)
                                                              / (MUME_MINUTES_PER_MOON_CYCLE
                                                                 - MUME_MINUTES_PER_DAY);

enum class NODISCARD MumeTimeEnum : uint8_t { UNKNOWN = 0, DAWN, DAY, DUSK, NIGHT };
enum class NODISCARD MumeSeasonEnum : uint8_t { UNKNOWN = 0, WINTER, SPRING, SUMMER, AUTUMN };

enum class NODISCARD MumeMoonPhaseEnum : int8_t {
    UNKNOWN = -1,    // Illumination Levels
    WAXING_CRESCENT, //  3,  4,  5
    FIRST_QUARTER,   //  6,  7,  8
    WAXING_GIBBOUS,  //  9,  10, 11
    FULL_MOON,       //  12
    WANING_GIBBOUS,  //  11, 10, 9
    THIRD_QUARTER,   //  8,  7,  6
    WANING_CRESCENT, //  5,  4,  3
    NEW_MOON         //  2,  1,  0
};
enum class MumeMoonVisibilityEnum : uint8_t { UNKNOWN, INVISIBLE, DIM, BRIGHT };
enum class NODISCARD MumeMoonPositionEnum : int8_t {
    UNKNOWN = -1,
    INVISIBLE,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST
};
static_assert(static_cast<int>(MumeMoonPositionEnum::EAST) == 1);
static_assert(static_cast<int>(MumeMoonPositionEnum::WEST) == 5);

class NODISCARD MumeMoment
{
public:
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;

public:
    explicit MumeMoment(int year, int month, int day, int hour, int minute);
    NODISCARD static MumeMoment sinceMumeEpoch(int64_t secsSinceMumeStartEpoch);

    NODISCARD int dayOfYear() const;
    NODISCARD int weekDay() const;
    NODISCARD int toSeconds() const;
    NODISCARD MumeSeasonEnum toSeason() const;
    NODISCARD MumeTimeEnum toTimeOfDay() const;

    NODISCARD int moonZenithMinutes() const;
    NODISCARD int moonLevel() const;
    NODISCARD MumeMoonPhaseEnum moonPhase() const;
    NODISCARD MumeMoonVisibilityEnum moonVisibility() const;
    NODISCARD MumeMoonPositionEnum moonPosition() const;
    NODISCARD int untilMoonPosition(MumeMoonPositionEnum pos) const;
    NODISCARD int untilMoonPhase(MumeMoonPhaseEnum phase) const;
    NODISCARD QString toMumeMoonTime() const;
    NODISCARD QString toMoonVisibilityCountDown() const;
    NODISCARD bool isMoonWaxing() const;
    NODISCARD bool isMoonBelowHorizon() const
    {
        return moonPosition() == MumeMoonPositionEnum::INVISIBLE;
    }
    NODISCARD bool isMoonBright() const { return moonLevel() > 4; }
};
