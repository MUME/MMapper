#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "mm_source_location.h"

NORETURN
extern void test_assert_fail(mm::source_location loc, const char *reason);
#define TEST_ASSERT(x) \
    do { \
        if (!(x)) { \
            test_assert_fail(MM_SOURCE_LOCATION(), #x); \
        } \
    } while (false)
