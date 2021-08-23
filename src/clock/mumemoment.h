#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>
#include <QString>

#include "../global/macros.h"

static constexpr const int MUME_START_YEAR = 2850;

enum class NODISCARD MumeTimeEnum { UNKNOWN = 0, DAWN, DAY, DUSK, NIGHT };
enum class NODISCARD MumeSeasonEnum { UNKNOWN = 0, WINTER, SPRING, SUMMER, AUTUMN };

enum class NODISCARD MumeMoonPhaseEnum {
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
enum class NODISCARD MumeMoonVisibilityEnum { POSITION_UNKNOWN = -1, HIDDEN, RISE, VISIBLE, SET };

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

    NODISCARD int moonCycle() const;
    NODISCARD int moonRise() const;
    NODISCARD int moonSet() const;
    NODISCARD int moonPosition() const;
    NODISCARD int moonLevel() const;
    NODISCARD MumeMoonPhaseEnum toMoonPhase() const;
    NODISCARD MumeMoonVisibilityEnum toMoonVisibility() const;
    NODISCARD QString toMumeMoonTime() const;
    NODISCARD QString toMoonCountDown() const;
    NODISCARD bool isMoonVisible() const
    {
        return toMoonVisibility() > MumeMoonVisibilityEnum::HIDDEN;
    }
    NODISCARD bool isMoonBright() const { return moonLevel() > 4; }
};
