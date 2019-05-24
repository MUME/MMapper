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

#include "RoomFieldVariant.h"

#include <cstring>
#include <new>
#include <stdexcept>

RoomFieldVariant::RoomFieldVariant(const RoomFieldVariant &rhs)
    : type_{rhs.type_}
{
    switch (type_) {
#define X_CASE(UPPER_CASE, CamelCase, Type) \
    do { \
    case RoomField::UPPER_CASE: \
        new (&storage_) Type(rhs.get##CamelCase()); \
        return; \
    } while (false);
        X_FOREACH_ROOM_FIELD(X_CASE)
#undef X_CASE
    case RoomField::NAME:
    case RoomField::DESC:
    case RoomField::DYNAMIC_DESC:
    case RoomField::LAST:
    case RoomField::RESERVED:
        break;
    }
    throw std::runtime_error("bad type");
}

RoomFieldVariant &RoomFieldVariant::operator=(const RoomFieldVariant &rhs)
{
    // FIXME: This is not kosher.
    this->~RoomFieldVariant();
    new (this) RoomFieldVariant(rhs);
    return *this;
}

#define X_DECLARE_CONSTRUCTORS(UPPER_CASE, CamelCase, Type) \
    RoomFieldVariant::RoomFieldVariant(Type type) \
        : type_{RoomField::UPPER_CASE} \
    { \
        new (&storage_) Type(std::move(type)); \
    }
X_FOREACH_ROOM_FIELD(X_DECLARE_CONSTRUCTORS)
#undef X_DECLARE_CONSTRUCTORS

RoomFieldVariant::~RoomFieldVariant()
{
    switch (type_) {
#define X_CASE(UPPER_CASE, CamelCase, Type) \
    { \
    case RoomField::UPPER_CASE: \
        reinterpret_cast<Type *>(&storage_)->~Type(); \
        break; \
    }
        X_FOREACH_ROOM_FIELD(X_CASE)
#undef X_CASE
    case RoomField::NAME:
    case RoomField::DESC:
    case RoomField::DYNAMIC_DESC:
    case RoomField::LAST:
    case RoomField::RESERVED:
        break;
    }

#ifndef NDEBUG
    // only useful for debugging
    ::memset(&storage_, 0, sizeof(storage_));
#endif
}

#define X_DECLARE_ACCESSORS(UPPER_CASE, CamelCase, Type) \
    Type RoomFieldVariant::get##CamelCase() const \
    { \
        if (type_ != RoomField::UPPER_CASE) \
            throw std::runtime_error("bad type"); \
        return *reinterpret_cast<const Type *>(&storage_); \
    }
X_FOREACH_ROOM_FIELD(X_DECLARE_ACCESSORS)
#undef X_DECLARE_ACCESSORS
