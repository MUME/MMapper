#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

// This macro only exists to keep clang-format from losing its mind
// when it encounters a c++11 attribute.
#define NODISCARD [[nodiscard]]
#define DEPRECATED [[deprecated]]
#define FALLTHRU [[fallthrough]]
