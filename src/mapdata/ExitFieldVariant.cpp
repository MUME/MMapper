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
    : type_{rhs.type_}
{
    switch (type_) {
    case ExitField::DOOR_NAME:
        new (&storage_) DoorName{rhs.getDoorName()};
        return;
    case ExitField::EXIT_FLAGS:
        new (&storage_) ExitFlags{rhs.getExitFlags()};
        return;
    case ExitField::DOOR_FLAGS:
        new (&storage_) DoorFlags{rhs.getDoorFlags()};
        return;
    }
    throw std::runtime_error("bad type");
}

ExitFieldVariant &ExitFieldVariant::operator=(const ExitFieldVariant &rhs)
{
    // FIXME: This is not kosher.
    this->~ExitFieldVariant();
    new (this) ExitFieldVariant(rhs);
    return *this;
}

ExitFieldVariant::ExitFieldVariant(DoorName doorName)
    : type_{ExitField::DOOR_NAME}
{
    new (&storage_) DoorName{std::move(doorName)};
}

ExitFieldVariant::ExitFieldVariant(const ExitFlags exitFlags)
    : type_{ExitField::EXIT_FLAGS}
{
    new (&storage_) ExitFlags{exitFlags};
}

ExitFieldVariant::ExitFieldVariant(const DoorFlags doorFlags)
    : type_{ExitField::DOOR_FLAGS}
{
    new (&storage_) DoorFlags{doorFlags};
}

ExitFieldVariant::~ExitFieldVariant()
{
    switch (type_) {
    case ExitField::DOOR_NAME:
        reinterpret_cast<DoorName *>(&storage_)->~DoorName();
        break;
    case ExitField::EXIT_FLAGS:
        reinterpret_cast<ExitFlags *>(&storage_)->~ExitFlags();
        break;
    case ExitField::DOOR_FLAGS:
        reinterpret_cast<DoorFlags *>(&storage_)->~DoorFlags();
        break;
    }

#ifndef NDEBUG
    // only useful for debugging
    ::memset(&storage_, 0, sizeof(storage_));
#endif
}

DoorName ExitFieldVariant::getDoorName() const
{
    if (type_ != ExitField::DOOR_NAME)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const DoorName *>(&storage_);
}

ExitFlags ExitFieldVariant::getExitFlags() const
{
    if (type_ != ExitField::EXIT_FLAGS)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const ExitFlags *>(&storage_);
}

DoorFlags ExitFieldVariant::getDoorFlags() const
{
    if (type_ != ExitField::DOOR_FLAGS)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const DoorFlags *>(&storage_);
}
