#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/StringView.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string_view>

NODISCARD bool isAbbrev(StringView input, std::string_view command, int minAbbrev);

struct NODISCARD Abbrev final
{
private:
    const char *m_command = nullptr;
    int m_minAbbrev = 0;
    int m_len = 0;

public:
    Abbrev() = default;
    explicit Abbrev(std::nullptr_t) = delete;
    explicit Abbrev(std::nullptr_t, int) = delete;
    explicit Abbrev(const char *arg_command, int arg_minAbbrev = -1);

    NODISCARD bool matches(StringView input) const
    {
        return ::isAbbrev(input, m_command, m_minAbbrev);
    }
    explicit operator bool() const;
    NODISCARD const char *getCommand() const { return m_command; }
    NODISCARD int getMinAbbrev() const { return m_minAbbrev; }
    NODISCARD int getLength() const { return m_len; }
    NODISCARD QString describe() const;
};
