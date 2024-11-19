// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "PromptFlags.h"

NODISCARD std::string_view to_string_view(const PromptFogEnum val)
{
#define CASE(X) \
    case PromptFogEnum::X: \
        return #X;
    switch (val) {
        X_FOREACH_PROMPT_FOG_ENUM(CASE)
    }
    std::abort();
#undef CASE
}
NODISCARD std::string_view to_string_view(const PromptWeatherEnum val)
{
#define CASE(X) \
    case PromptWeatherEnum::X: \
        return #X;
    switch (val) {
        X_FOREACH_PROMPT_WEATHER_ENUM(CASE)
    }
    std::abort();
#undef CASE
}
