#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <string>

#include "../global/RuleOf5.h"
#include "listcycler.h"

class Property final : public ListCycler<char, std::string>
{
public:
    using ListCycler::ListCycler;

public:
    const char *rest() const;
    inline bool isSkipped() const noexcept { return m_skipped; }

public:
    static constexpr const struct TagSkip
    {
    } tagSkip{};
    explicit Property(TagSkip);
    virtual ~Property() override;
    DEFAULT_CTORS_AND_ASSIGN_OPS(Property);

private:
    bool m_skipped = false;
};
