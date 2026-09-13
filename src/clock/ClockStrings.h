#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"
#include "../map/PromptFlags.h"
#include "mumemoment.h"

#include <QString>

// Pure enum -> user-facing string mappings used by MumeClockWidget (the
// QWidget-based clock). Kept free of any Qt widget dependency.
namespace clockstrings {

// Emoji glyph for the moon phase, or an empty string for UNKNOWN.
NODISCARD QString moonPhaseEmoji(MumeMoonPhaseEnum phase);

// Display text for the season, or an empty string for UNKNOWN.
NODISCARD QString seasonText(MumeSeasonEnum season);

// Emoji glyph for the weather, or an empty string for NICE (nothing to show).
NODISCARD QString weatherEmoji(PromptWeatherEnum weather);
// Human-readable tooltip/status-tip text for the weather, or an empty string for NICE.
NODISCARD QString weatherTooltip(PromptWeatherEnum weather);

// Emoji glyph(s) for the fog, or an empty string for NO_FOG.
NODISCARD QString fogEmoji(PromptFogEnum fog);
// Human-readable tooltip/status-tip text for the fog, or an empty string for NO_FOG.
NODISCARD QString fogTooltip(PromptFogEnum fog);

} // namespace clockstrings
