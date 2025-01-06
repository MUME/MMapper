// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "PromptFlags.h"

NODISCARD std::string_view to_string_view(const PromptFogEnum val)
{
#define X_CASE(X) \
    case PromptFogEnum::X: \
        return #X;
    switch (val) {
        XFOREACH_PROMPT_FOG_ENUM(X_CASE)
    }
    std::abort();
#undef X_CASE
}
NODISCARD std::string_view to_string_view(const PromptWeatherEnum val)
{
#define X_CASE(X) \
    case PromptWeatherEnum::X: \
        return #X;
    switch (val) {
        XFOREACH_PROMPT_WEATHER_ENUM(X_CASE)
    }
    std::abort();
#undef X_CASE
}
