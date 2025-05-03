// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "RawExit.h"

#include "../global/macros.h"

namespace detail {

/*
template<typename T>
struct is_const_ref : std::bool_constant<!std::is_const_v<std::remove_reference_t<T>>>
{};
*/

template<typename Exit_>
struct NODISCARD InvariantsHelper final
{
    Exit_ &exit;
    const ExitFlags &flags = exit.getExitFlags();
    const bool hasAnyExits = !exit.outgoing.empty();
    const bool hasAnyDoorFlags = !exit.getDoorFlags().empty();
    const bool hasDoorName = !exit.getDoorName().empty();

    const bool hasActualExitFlag = flags.isExit();
    const bool hasActualDoorFlag = flags.isDoor();
    const bool hasActualUnmappedFlag = flags.isUnmapped();

    const bool shouldHaveUnmappedFlag = !hasAnyExits && hasActualExitFlag;
    const bool shouldHaveExitFlag = hasAnyExits || shouldHaveUnmappedFlag;
    const bool shouldHaveDoorFlag = shouldHaveExitFlag
                                    && (hasActualDoorFlag || hasAnyDoorFlags || hasDoorName);

    explicit InvariantsHelper(Exit_ &exit_)
        : exit{exit_}
    {}

    NODISCARD bool satisfied() const
    {
        if (hasActualExitFlag != shouldHaveExitFlag) {
            return false;
        }
        if (hasActualDoorFlag != shouldHaveDoorFlag) {
            return false;
        }
        if ((hasAnyDoorFlags || hasDoorName) && !shouldHaveDoorFlag) {
            return false;
        }
        if (hasActualUnmappedFlag != shouldHaveUnmappedFlag) {
            return false;
        }
        return true;
    }

    // Note: This function will cause a ton of compiler errors if you try to call it
    // with a const ref Exit type, but we can't use std::enable_if_t here.
    void enforce()
    {
        if (hasActualExitFlag != shouldHaveExitFlag) {
            if (shouldHaveExitFlag) {
                exit.addExitFlags(ExitFlagEnum::EXIT);
            } else {
                exit.removeExitFlags(ExitFlagEnum::EXIT);
            }
        }

        if (hasActualDoorFlag != shouldHaveDoorFlag) {
            if (shouldHaveDoorFlag) {
                exit.addExitFlags(ExitFlagEnum::DOOR);
            } else {
                exit.removeExitFlags(ExitFlagEnum::DOOR);
            }
        }

        if (hasAnyDoorFlags && !shouldHaveDoorFlag) {
            exit.setDoorFlags({});
        }
        if (hasDoorName && !shouldHaveDoorFlag) {
            exit.setDoorName({});
        }

        if (hasActualUnmappedFlag != shouldHaveUnmappedFlag) {
            if (shouldHaveUnmappedFlag) {
                exit.addExitFlags(ExitFlagEnum::UNMAPPED);
            } else {
                exit.removeExitFlags(ExitFlagEnum::UNMAPPED);
            }
        }
    }
};

template<typename Exit_>
NODISCARD static bool satisfiesInvariants(const Exit_ &exit)
{
    return InvariantsHelper<const Exit_>(exit).satisfied();
}

template<typename Exit_>
static void enforceInvariants(Exit_ &exit)
{
    InvariantsHelper<Exit_>(exit).enforce();
    assert(satisfiesInvariants(exit));
}
} // namespace detail

bool satisfiesInvariants(const RawExit &e)
{
    return detail::satisfiesInvariants(e);
}
bool satisfiesInvariants(const ExternalRawExit &e)
{
    return detail::satisfiesInvariants(e);
}

void enforceInvariants(RawExit &e)
{
    detail::enforceInvariants(e);
}
void enforceInvariants(ExternalRawExit &e)
{
    detail::enforceInvariants(e);
}
