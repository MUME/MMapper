// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "ClockStrings.h"

namespace clockstrings {

QString moonPhaseEmoji(const MumeMoonPhaseEnum phase)
{
    switch (phase) {
    case MumeMoonPhaseEnum::WAXING_CRESCENT:
        return QString::fromUtf8("\xF0\x9F\x8C\x92");
    case MumeMoonPhaseEnum::FIRST_QUARTER:
        return QString::fromUtf8("\xF0\x9F\x8C\x93");
    case MumeMoonPhaseEnum::WAXING_GIBBOUS:
        return QString::fromUtf8("\xF0\x9F\x8C\x94");
    case MumeMoonPhaseEnum::FULL_MOON:
        return QString::fromUtf8("\xF0\x9F\x8C\x95");
    case MumeMoonPhaseEnum::WANING_GIBBOUS:
        return QString::fromUtf8("\xF0\x9F\x8C\x96");
    case MumeMoonPhaseEnum::THIRD_QUARTER:
        return QString::fromUtf8("\xF0\x9F\x8C\x97");
    case MumeMoonPhaseEnum::WANING_CRESCENT:
        return QString::fromUtf8("\xF0\x9F\x8C\x98");
    case MumeMoonPhaseEnum::NEW_MOON:
        return QString::fromUtf8("\xF0\x9F\x8C\x91");
    case MumeMoonPhaseEnum::UNKNOWN:
    default:
        return QString();
    }
}

QString seasonText(const MumeSeasonEnum season)
{
    switch (season) {
    case MumeSeasonEnum::WINTER:
        return QStringLiteral("Winter");
    case MumeSeasonEnum::SPRING:
        return QStringLiteral("Spring");
    case MumeSeasonEnum::SUMMER:
        return QStringLiteral("Summer");
    case MumeSeasonEnum::AUTUMN:
        return QStringLiteral("Autumn");
    case MumeSeasonEnum::UNKNOWN:
    default:
        return QString();
    }
}

QString weatherEmoji(const PromptWeatherEnum weather)
{
    switch (weather) {
    case PromptWeatherEnum::CLOUDS:
        return QString::fromUtf8("\xE2\x98\x81");
    case PromptWeatherEnum::RAIN:
        return QString::fromUtf8("\xF0\x9F\x8C\xA7");
    case PromptWeatherEnum::HEAVY_RAIN:
        return QString::fromUtf8("\xE2\x9B\x88");
    case PromptWeatherEnum::SNOW:
        return QString::fromUtf8("\xE2\x9D\x84");
    case PromptWeatherEnum::NICE:
    default:
        return QString();
    }
}

QString weatherTooltip(const PromptWeatherEnum weather)
{
    switch (weather) {
    case PromptWeatherEnum::CLOUDS:
        return QStringLiteral("Cloudy");
    case PromptWeatherEnum::RAIN:
        return QStringLiteral("Rainy");
    case PromptWeatherEnum::HEAVY_RAIN:
        return QStringLiteral("Heavy Rain");
    case PromptWeatherEnum::SNOW:
        return QStringLiteral("Snowy");
    case PromptWeatherEnum::NICE:
    default:
        return QString();
    }
}

QString fogEmoji(const PromptFogEnum fog)
{
    switch (fog) {
    case PromptFogEnum::LIGHT_FOG:
        return QString::fromUtf8("\xF0\x9F\x8C\xAB");
    case PromptFogEnum::HEAVY_FOG:
        return QString::fromUtf8("\xF0\x9F\x8C\xAB\xF0\x9F\x8C\xAB");
    case PromptFogEnum::NO_FOG:
    default:
        return QString();
    }
}

QString fogTooltip(const PromptFogEnum fog)
{
    switch (fog) {
    case PromptFogEnum::LIGHT_FOG:
        return QStringLiteral("Light Fog");
    case PromptFogEnum::HEAVY_FOG:
        return QStringLiteral("Heavy Fog");
    case PromptFogEnum::NO_FOG:
    default:
        return QString();
    }
}

} // namespace clockstrings
