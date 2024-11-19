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
    const char *command = nullptr;
    int minAbbrev = 0;
    int len = 0;

public:
    Abbrev() = default;
    explicit Abbrev(std::nullptr_t) = delete;
    explicit Abbrev(std::nullptr_t, int) = delete;
    explicit Abbrev(const char *arg_command, int arg_minAbbrev = -1);

    NODISCARD bool matches(StringView input) const { return ::isAbbrev(input, command, minAbbrev); }
    explicit operator bool() const;
    NODISCARD const char *getCommand() const { return command; }
    NODISCARD int getMinAbbrev() const { return minAbbrev; }
    NODISCARD int getLength() const { return len; }
    NODISCARD QString describe() const;
};
