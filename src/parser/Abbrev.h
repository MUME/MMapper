#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_ABBREV_H
#define MMAPPER_ABBREV_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <QCharRef>

#include "../global/StringView.h"

bool isAbbrev(StringView input, const char *const command, const int minAbbrev);

struct Abbrev
{
private:
    const char *command = nullptr;
    int minAbbrev = 0;
    int len = 0;

public:
    explicit Abbrev() = default;
    explicit Abbrev(std::nullptr_t) = delete;
    explicit Abbrev(std::nullptr_t, int) = delete;
    explicit Abbrev(const char *const arg_command, const int arg_minAbbrev = -1);

    bool matches(StringView input) const { return ::isAbbrev(input, command, minAbbrev); }
    explicit operator bool() const;
    const char *getCommand() const { return command; }
    int getMinAbbrev() const { return minAbbrev; }
    int getLength() const { return len; }
    QString describe() const;
};

#endif // MMAPPER_ABBREV_H
