#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"
#include "mm_source_location.h"

namespace thread_utils {

NODISCARD extern bool isOnMainThread();

#define ABORT_IF_NOT_ON_MAIN_THREAD() ::thread_utils::abortIfNotOnMainThread(MM_SOURCE_LOCATION())
extern void abortIfNotOnMainThread(mm::source_location loc);

} // namespace thread_utils
