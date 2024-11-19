#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"

#include <iosfwd>
#include <ostream>

struct NODISCARD User
{
private:
    std::ostream &os;

public:
    explicit User(std::ostream &os)
        : os(os)
    {}

    NODISCARD std::ostream &getOstream() const { return os; }
};
