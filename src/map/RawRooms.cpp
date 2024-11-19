// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "RawRooms.h"

#include "../global/logging.h"

namespace { // anonymous

void update(ExitFlags &flags, const ExitFlagEnum flag, const bool shouldHaveFlag)
{
    if (shouldHaveFlag) {
        flags.insert(flag);
    } else {
        flags.remove(flag);
    }
}

} // namespace

void RawRooms::setExitFlags_safe(const RoomId id, const ExitDirEnum dir, const ExitFlags input_flags)
{
    // MAYBE_UNUSED static constexpr const ExitFlagEnum flag = ExitFlagEnum::EXIT;
    auto flags = input_flags;

    const bool hasExits = !getExitOutgoing(id, dir).empty();
    const bool hasDoorFlags = !getExitDoorFlags(id, dir).empty();
    const bool hasDoorName = !getExitDoorName(id, dir).empty();
    const bool hasActualDoorFlag = flags.isDoor();

    const bool shouldHaveExitFlag = hasExits;
    const bool shouldHaveDoorFlag = shouldHaveExitFlag
                                    && (hasActualDoorFlag || hasDoorFlags || hasDoorName);

    update(flags, ExitFlagEnum::EXIT, shouldHaveExitFlag);
    update(flags, ExitFlagEnum::DOOR, shouldHaveDoorFlag);

    constexpr bool extraDebug = false;
    if (extraDebug) {
        if (flags != input_flags) {
            std::ostringstream os;
            os << "WARNING: Auto-modified exit flags for internal roomid " << id.value() << " "
               << to_string_view(dir) << ":";

            const auto removed = input_flags & ~flags;
            if (removed) {
                os << " Removed[";
                removed.for_each([&os](auto f) { os << " " << to_string_view(f); });
                os << "]";
            }
            const auto added = (flags & ~input_flags);
            if (added) {
                os << " Added[";
                added.for_each([&os](auto f) { os << " " << to_string_view(f); });
                os << "]";
            }

            os << "\n";
            MMLOG() << std::move(os).str();
        }
    }

    setExitExitFlags(id, dir, flags);
    updateDoorFlags(id, dir);
}

void RawRooms::updateExitFlags(const RoomId id, const ExitDirEnum dir)
{
    setExitFlags_safe(id, dir, getExitFlags(id, dir));
}

void RawRooms::updateDoorFlags(const RoomId id, const ExitDirEnum dir)
{
    if (getExitExitFlags(id, dir).isDoor()) {
        return;
    }

    if (!getExitDoorFlags(id, dir).empty()) {
        if ((false)) {
            MMLOG() << "WARNING: Auto-removed door flags for internal roomid " << id.value() << " "
                    << to_string_view(dir) << ".";
        }
        setExitDoorFlags(id, dir, DoorFlags{});
    }

    if (!getExitDoorName(id, dir).empty()) {
        // REVISIT: append it to the RoomNote?
        if ((false)) {
            MMLOG() << "WARNING: Auto-removed door name for internal room " << id.value() << " "
                    << to_string_view(dir) << ".";
        }
        setExitDoorName(id, dir, DoorName{});
    }
}
