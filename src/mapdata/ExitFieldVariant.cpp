// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    case ExitFieldEnum::DOOR_NAME:
        new (&storage_) DoorName{rhs.getDoorName()};
        return;
    case ExitFieldEnum::EXIT_FLAGS:
        new (&storage_) ExitFlags{rhs.getExitFlags()};
        return;
    case ExitFieldEnum::DOOR_FLAGS:
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
    : type_{ExitFieldEnum::DOOR_NAME}
{
    new (&storage_) DoorName{std::move(doorName)};
}

ExitFieldVariant::ExitFieldVariant(const ExitFlags exitFlags)
    : type_{ExitFieldEnum::EXIT_FLAGS}
{
    new (&storage_) ExitFlags{exitFlags};
}

ExitFieldVariant::ExitFieldVariant(const DoorFlags doorFlags)
    : type_{ExitFieldEnum::DOOR_FLAGS}
{
    new (&storage_) DoorFlags{doorFlags};
}

ExitFieldVariant::~ExitFieldVariant()
{
    switch (type_) {
    case ExitFieldEnum::DOOR_NAME:
        reinterpret_cast<DoorName *>(&storage_)->~DoorName();
        break;
    case ExitFieldEnum::EXIT_FLAGS:
        reinterpret_cast<ExitFlags *>(&storage_)->~ExitFlags();
        break;
    case ExitFieldEnum::DOOR_FLAGS:
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
    if (type_ != ExitFieldEnum::DOOR_NAME)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const DoorName *>(&storage_);
}

ExitFlags ExitFieldVariant::getExitFlags() const
{
    if (type_ != ExitFieldEnum::EXIT_FLAGS)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const ExitFlags *>(&storage_);
}

DoorFlags ExitFieldVariant::getDoorFlags() const
{
    if (type_ != ExitFieldEnum::DOOR_FLAGS)
        throw std::runtime_error("bad type");
    return *reinterpret_cast<const DoorFlags *>(&storage_);
}
