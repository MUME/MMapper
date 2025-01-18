// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "tests.h"

#include <deque>
#include <memory>
#include <mutex>

#include <QDebug>

NORETURN
void test_assert_fail(const mm::source_location loc, const char *const reason)
{
    QMessageLogger(loc.file_name(), static_cast<int>(loc.line()), loc.function_name())
        .fatal("test assertion failed: expression (%s) is false at %s:%d",
               (reason != nullptr) ? reason : "false",
               loc.file_name(),
               loc.line());

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif
    // in case fatal() somehow returns...
    // NOLINTNEXTLINE
    std::abort();
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
