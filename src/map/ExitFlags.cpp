// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "ExitFlags.h"

std::string_view to_string_view(const ExitFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case ExitFlagEnum::UPPER_CASE: \
        return #UPPER_CASE; \
    } while (false);
    switch (flag) {
        XFOREACH_EXIT_FLAG(X_CASE)
    }
    return "*BUG*";
#undef X_CASE
}

std::string_view getName(const ExitFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case ExitFlagEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (flag) {
        XFOREACH_EXIT_FLAG(X_CASE)
    }
    return "*BUG*";
#undef X_CASE
}
