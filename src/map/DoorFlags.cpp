// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "DoorFlags.h"

std::string_view to_string_view(const DoorFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case DoorFlagEnum::UPPER_CASE: \
        return #UPPER_CASE; \
    } while (false);
    switch (flag) {
        XFOREACH_DOOR_FLAG(X_CASE)
    }
    return "*BUG*";
#undef X_CASE
}

std::string_view getName(const DoorFlagEnum flag)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case DoorFlagEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (flag) {
        XFOREACH_DOOR_FLAG(X_CASE)
    }
    return "*BUG*";
#undef X_CASE
}
