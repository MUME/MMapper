#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2025 The MMapper Authors

#include "../global/macros.h"

#include <stdexcept>
#include <string>

struct NODISCARD MapConsistencyError final : public std::runtime_error
{
    explicit MapConsistencyError(const std::string &msg)
        : std::runtime_error("MapConsistencyError: " + msg)
    {}
    ~MapConsistencyError() final;
};
