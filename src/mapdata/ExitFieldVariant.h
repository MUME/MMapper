#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <QString>
#include <QtGlobal>

#include "../global/RuleOf5.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFlags.h"

using DoorName = QString;

enum class ExitField { DOOR_NAME = 0, EXIT_FLAGS, DOOR_FLAGS };
static constexpr const int NUM_EXIT_PROPS = 3;

/* REVISIT: Replace this hacky variant.
 *
 * std::variant isn't available until c++17, but clang can't compile
 * the basic std::variant examples, so it's not really an option even on c++17.
 *
 * Consider using boost's templated variant, or one from another open source project?
 */
class ExitFieldVariant final
{
private:
    static constexpr const size_t STORAGE_ALIGNMENT = std::max(alignof(DoorName),
                                                               std::max(alignof(ExitFlags),
                                                                        alignof(DoorFlags)));
    static constexpr const size_t STORAGE_SIZE = std::max(sizeof(DoorName),
                                                          std::max(sizeof(ExitFlags),
                                                                   sizeof(DoorFlags)));

    std::aligned_storage_t<STORAGE_SIZE, STORAGE_ALIGNMENT> storage_;
    static_assert(sizeof(storage_) >= STORAGE_SIZE);
    ExitField type_{};

public:
    ExitFieldVariant() = delete;

public:
    /* NOTE: Adding move support would be a pain for little gain.
     * You'd at least need a boolean indicating that the object
     * is in moved-from state, or a reserved enum value. */
    DELETE_MOVE_CTOR(ExitFieldVariant);
    DELETE_MOVE_ASSIGN_OP(ExitFieldVariant);

public:
    ExitFieldVariant(const ExitFieldVariant &rhs);
    ExitFieldVariant &operator=(const ExitFieldVariant &rhs);

public:
    explicit ExitFieldVariant(DoorName doorName);
    explicit ExitFieldVariant(ExitFlags exitFlags);
    explicit ExitFieldVariant(DoorFlags doorFlags);

    ~ExitFieldVariant();

    inline ExitField getType() const { return type_; }
    DoorName getDoorName() const;
    ExitFlags getExitFlags() const;
    DoorFlags getDoorFlags() const;
};
