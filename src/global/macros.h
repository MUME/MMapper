#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

// These macros only exists to keep clang-format from losing its mind
// when it encounters c++11 attributes.

#define DEPRECATED [[deprecated]]
#define DEPRECATED_MSG(_msg) [[deprecated(_msg)]]
#define FALLTHRU [[fallthrough]]
#define NODISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]
