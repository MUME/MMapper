#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <string>

namespace syntax {

struct NODISCARD IMatchErrorLogger
{
public:
    IMatchErrorLogger() = default;
    virtual ~IMatchErrorLogger();
    DEFAULT_CTORS_AND_ASSIGN_OPS(IMatchErrorLogger);

private:
    virtual void virt_logError(std::string s) = 0;

public:
    void logError(std::string s) { virt_logError(std::move(s)); }
};

} // namespace syntax
