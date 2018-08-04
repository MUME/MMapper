#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef DOOR_H
#define DOOR_H

#include <QString>

enum class OldDoorFlag {
    HIDDEN,
    NEEDKEY,
    NOBLOCK,
    NOBREAK,
    NOPICK,
    DELAYED,
};

static constexpr const int NUM_OLD_DOOR_FLAGS = static_cast<int>(OldDoorFlag::DELAYED) + 1;
static_assert(NUM_OLD_DOOR_FLAGS == 6, "");
DEFINE_ENUM_COUNT(OldDoorFlag, NUM_OLD_DOOR_FLAGS);

class OldDoorFlags final : public enums::Flags<OldDoorFlags, OldDoorFlag, uint16_t>
{
    using Flags::Flags;
};

using DoorName = QString;

class Door
{
public:
    explicit Door(DoorName name = DoorName{}, OldDoorFlags flags = OldDoorFlags{})
        : m_name{name}
        , m_flags{flags}
    {}

    OldDoorFlags getFlags() const { return m_flags; };
    const DoorName &getName() const { return m_name; };
    void setFlags(OldDoorFlags flags) { m_flags = flags; };
    void setName(const DoorName &name) { m_name = name; };

private:
    DoorName m_name{};
    OldDoorFlags m_flags{};
};

#endif
