// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    case RoomFieldEnum::UPPER_CASE: \
        new (&storage_) Type(rhs.get##CamelCase()); \
        return; \
    } while (false);
        X_FOREACH_ROOM_FIELD(X_CASE)
#undef X_CASE
    case RoomFieldEnum::NAME:
    case RoomFieldEnum::DESC:
    case RoomFieldEnum::DYNAMIC_DESC:
    case RoomFieldEnum::LAST:
    case RoomFieldEnum::RESERVED:
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
        : type_{RoomFieldEnum::UPPER_CASE} \
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
    case RoomFieldEnum::UPPER_CASE: \
        reinterpret_cast<Type *>(&storage_)->~Type(); \
        break; \
    }
        X_FOREACH_ROOM_FIELD(X_CASE)
#undef X_CASE
    case RoomFieldEnum::NAME:
    case RoomFieldEnum::DESC:
    case RoomFieldEnum::DYNAMIC_DESC:
    case RoomFieldEnum::LAST:
    case RoomFieldEnum::RESERVED:
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
        if (type_ != RoomFieldEnum::UPPER_CASE) \
            throw std::runtime_error("bad type"); \
        return *reinterpret_cast<const Type *>(&storage_); \
    }
X_FOREACH_ROOM_FIELD(X_DECLARE_ACCESSORS)
#undef X_DECLARE_ACCESSORS
