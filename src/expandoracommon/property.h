#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <optional>
#include <string>

class NODISCARD Property final
{
private:
    std::string m_data;

public:
    bool isSkipped() const noexcept { return m_data.empty(); }
    const std::string &getStdString() const { return m_data; }
    size_t size() const { return m_data.size(); }

public:
    Property() = default;
    explicit Property(std::string s);
    ~Property();
    DEFAULT_CTORS_AND_ASSIGN_OPS(Property);
};
