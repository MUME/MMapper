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

#include "ExitFieldVariant.h"

#include <cstring>
#include <new>
#include <stdexcept>

#include "DoorFlags.h"
#include "ExitFlags.h"

ExitFieldVariant::ExitFieldVariant(const ExitFieldVariant &rhs)
    : type{rhs.type}
{
    switch (type) {
    case ExitField::DOOR_NAME:
        new (storage) DoorName(rhs.getDoorName());
        break;
    case ExitField::EXIT_FLAGS:
        new (storage) ExitFlags(rhs.getExitFlags());
        break;
    case ExitField::DOOR_FLAGS:
        new (storage) DoorFlags(rhs.getDoorFlags());
        break;
    default:
        throw std::runtime_error("bad type");
    }
}

ExitFieldVariant &ExitFieldVariant::operator=(const ExitFieldVariant &rhs)
{
    this->~ExitFieldVariant();
    new (this) ExitFieldVariant(rhs);
    return *this;
}

ExitFieldVariant::ExitFieldVariant(DoorName doorName)
    : type{ExitField::DOOR_NAME}
{
    new (storage) DoorName(std::move(doorName));
}

ExitFieldVariant::ExitFieldVariant(const ExitFlags exitFlags)
    : type{ExitField::EXIT_FLAGS}
{
    new (storage) ExitFlags(exitFlags);
}

ExitFieldVariant::ExitFieldVariant(const DoorFlags doorFlags)
    : type{ExitField::DOOR_FLAGS}
{
    new (storage) DoorFlags(doorFlags);
}

ExitFieldVariant::~ExitFieldVariant()
{
    switch (type) {
    case ExitField::DOOR_NAME:
        reinterpret_cast<DoorName *>(storage)->~DoorName();
        break;
    case ExitField::EXIT_FLAGS:
        reinterpret_cast<ExitFlags *>(storage)->~ExitFlags();
        break;
    case ExitField::DOOR_FLAGS:
        reinterpret_cast<DoorFlags *>(storage)->~DoorFlags();
        break;
    }
    ::memset(storage, 0, sizeof(storage));
}

DoorName ExitFieldVariant::getDoorName() const
{
    if (type != ExitField::DOOR_NAME)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const DoorName *>(storage);
}

ExitFlags ExitFieldVariant::getExitFlags() const
{
    if (type != ExitField::EXIT_FLAGS)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const ExitFlags *>(storage);
}

DoorFlags ExitFieldVariant::getDoorFlags() const
{
    if (type != ExitField::DOOR_FLAGS)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const DoorFlags *>(storage);
}
