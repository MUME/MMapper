#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/AnsiOstream.h"
#include "../global/macros.h"

struct NODISCARD User
{
private:
    AnsiOstream &m_aos;

public:
    explicit User(AnsiOstream &aos)
        : m_aos{aos}
    {}

    NODISCARD AnsiOstream &getOstream() const { return m_aos; }
};
