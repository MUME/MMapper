// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "InvalidMapOperation.h"

#include <string>

InvalidMapOperation::InvalidMapOperation(const std::string_view message)
    : std::runtime_error(std::string(message))
{}

InvalidMapOperation::~InvalidMapOperation() = default;
