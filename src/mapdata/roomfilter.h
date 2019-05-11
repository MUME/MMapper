#pragma once
/************************************************************************
**
** Authors:   ethorondil
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

#ifndef ROOMFILTER_H
#define ROOMFILTER_H

#include <cassert>
#include <QString>
#include <QtCore>

#include "../expandoracommon/room.h"

class Room;

enum class pattern_kinds { NONE, DESC, DYN_DESC, NAME, NOTE, EXITS, FLAGS, ALL };
static constexpr const auto PATTERN_KINDS_LENGTH = static_cast<size_t>(pattern_kinds::ALL) + 1u;
static_assert(PATTERN_KINDS_LENGTH == 8, "");

class RoomFilter
{
public:
    explicit RoomFilter() = default;
    explicit RoomFilter(const QString &pattern,
                        const Qt::CaseSensitivity &cs,
                        const pattern_kinds kind)
        : pattern(pattern)
        , cs(cs)
        , kind(kind)
    {}

    // Return false on parse failure
    static bool parseRoomFilter(const QString &line, RoomFilter &output);
    static const char *parse_help;

    bool filter(const Room *r) const;
    pattern_kinds patternKind() const { return kind; }

protected:
    QString pattern{};
    Qt::CaseSensitivity cs{};
    pattern_kinds kind = pattern_kinds::NONE;
};

#endif
