#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"

#include <stdexcept>
#include <string_view>

struct NODISCARD InvalidMapOperation : public std::runtime_error
{
    explicit InvalidMapOperation(std::string_view message = "InvalidMapOperation");
    ~InvalidMapOperation() override;
};
