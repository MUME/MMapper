// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "RawRoom.h"

#include "../global/ConfigConsts.h"
#include "../global/logging.h"
#include "enums.h"
#include "sanitizer.h"

#include <iostream>

template<>
std::string RawRoom::toStdStringUtf8() const
{
    return ::toStdStringUtf8(*this);
}

template<>
std::string ExternalRawRoom::toStdStringUtf8() const
{
    return ::toStdStringUtf8(*this);
}

namespace sanitize_helper {

enum class NODISCARD ChangeEnum : uint8_t { None, Sanitized };

template<typename T>
static void sanitizeRoomField(ExternalRoomId id, T &value) = delete;

template<typename T>
static void sanitizeExitField(ExternalRoomId id, ExitDirEnum, T &value) = delete;

template<typename T>
NODISCARD static ChangeEnum sanitizeString(TaggedBoxedStringUtf8<T> &boxed)
{
    const std::string_view view = boxed.getStdStringViewUtf8();
    if constexpr (std::is_same_v<T, tags::RoomNameTag> || std::is_same_v<T, tags::DoorNameTag>) {
        if (sanitizer::isSanitizedOneLine(view)) {
            return ChangeEnum::None;
        }

        boxed = TaggedBoxedStringUtf8<T>{sanitizer::sanitizeOneLine(std::string{view})};
        return ChangeEnum::Sanitized;
    } else if constexpr (std::is_same_v<T, tags::RoomDescTag> //
                         || std::is_same_v<T, tags::RoomContentsTag>) {
        if (sanitizer::isSanitizedMultiline(view)) {
            return ChangeEnum::None;
        }

        boxed = TaggedBoxedStringUtf8<T>{sanitizer::sanitizeMultiline(std::string{view})};
        return ChangeEnum::Sanitized;
    } else if constexpr (std::is_same_v<T, tags::RoomNoteTag>) {
        if (sanitizer::isSanitizedUserSupplied(view)) {
            return ChangeEnum::None;
        }

        boxed = TaggedBoxedStringUtf8<T>{sanitizer::sanitizeUserSupplied(std::string{view})};
        return ChangeEnum::Sanitized;
    } else {
        std::abort();
    }
}

template<typename T>
void sanitizeRoomField(MAYBE_UNUSED const ExternalRoomId id, TaggedBoxedStringUtf8<T> &str)
{
    if (sanitizeString(str) != ChangeEnum::None) {
        // REVISIT: mention that a change occurred?
    }
}

template<typename T>
void sanitizeExitField(MAYBE_UNUSED const ExternalRoomId id,
                       MAYBE_UNUSED ExitDirEnum dir,
                       TaggedBoxedStringUtf8<T> &str)
{
    if (sanitizeString(str) != ChangeEnum::None) {
        // REVISIT: mention that a change occurred?
    }
}

template<>
void sanitizeRoomField(MAYBE_UNUSED const ExternalRoomId id, RoomMobFlags &value)
{
    value = enums::sanitizeFlags(value);
}
template<>
void sanitizeRoomField(MAYBE_UNUSED const ExternalRoomId id, RoomLoadFlags &value)
{
    value = enums::sanitizeFlags(value);
}
template<>
void sanitizeExitField(const ExternalRoomId /*id*/, const ExitDirEnum /*dir*/, ExitFlags &value)
{
    value = enums::sanitizeFlags(value);
}

template<>
void sanitizeExitField(const ExternalRoomId /*id*/, const ExitDirEnum /*dir*/, DoorFlags &value)
{
    auto &flags = value; // parameter named value to avoid warning
    flags = enums::sanitizeFlags(flags);
}

template<>
void sanitizeRoomField(const ExternalRoomId /*id*/, RoomTerrainEnum &value)
{
    value = enums::sanitizeEnum(value);
}
template<>
void sanitizeRoomField(const ExternalRoomId /*id*/, RoomPortableEnum &value)
{
    value = enums::sanitizeEnum(value);
}
template<>
void sanitizeRoomField(const ExternalRoomId /*id*/, RoomLightEnum &value)
{
    value = enums::sanitizeEnum(value);
}
template<>
void sanitizeRoomField(const ExternalRoomId /*id*/, RoomAlignEnum &value)
{
    value = enums::sanitizeEnum(value);
}
template<>
void sanitizeRoomField(const ExternalRoomId /*id*/, RoomRidableEnum &value)
{
    value = enums::sanitizeEnum(value);
}
template<>
void sanitizeRoomField(const ExternalRoomId /*id*/, RoomSundeathEnum &value)
{
    value = enums::sanitizeEnum(value);
}

static void sanitize(ExternalRawExit &exit, const ExternalRoomId room, const ExitDirEnum dir)
{
#define X_SANITIZE(_Type, _Prop, _OptInit) \
    sanitize_helper::sanitizeExitField(room, dir, exit.fields._Prop);
    XFOREACH_EXIT_PROPERTY(X_SANITIZE)
#undef X_SANITIZE
}

static void sanitize(ExternalRawRoom &rawRoom)
{
    const ExternalRoomId room = rawRoom.getId();
    {
#define X_SANITIZE(_Type, _Prop, _OptInit) \
    sanitize_helper::sanitizeRoomField(room, rawRoom.fields._Prop);
        XFOREACH_ROOM_PROPERTY(X_SANITIZE)
#undef X_SANITIZE
    }

    for (const ExitDirEnum dir : ALL_EXITS7) {
        ExternalRawExit &exit = rawRoom.exits[dir];
        sanitize(exit, room, dir); // this is just for illegal enum values
        enforceInvariants(exit);   // this checks if EXIT and DOOR flags are correct.
    }
}
} // namespace sanitize_helper

void sanitize(ExternalRawRoom &raw)
{
    sanitize_helper::sanitize(raw);
}

namespace detail {
template<typename Room_>
NODISCARD static bool satisfiesInvariants(Room_ &room)
{
    for (auto &exit : room.getExits()) {
        if (!::satisfiesInvariants(exit)) {
            return false;
        }
    }
    return true;
}
template<typename Room_>
static void enforceInvariants(Room_ &room)
{
    for (auto &exit : room.getExits()) {
        ::enforceInvariants(exit);
    }
}
} // namespace detail

bool satisfiesInvariants(const RawRoom &room)
{
    return detail::satisfiesInvariants(room);
}

bool satisfiesInvariants(const ExternalRawRoom &room)
{
    return detail::satisfiesInvariants(room);
}

void enforceInvariants(RawRoom &room)
{
    detail::enforceInvariants(room);
}

void enforceInvariants(ExternalRawRoom &room)
{
    detail::enforceInvariants(room);
}

ExitDirFlags computeExitDirections(const RawRoom &room)
{
    ExitDirFlags result;
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const auto &flags = room.getExitFlags(dir);
        if (flags.isExit()) {
            result |= dir;
        }
    }

    return result;
}

ExitsFlagsType computeExitsFlags(const RawRoom &room)
{
    ExitsFlagsType tmp;
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const auto flags = room.getExitFlags(dir);
        if (flags.isExit()) {
            tmp.set(dir, flags);
        }
    }
    tmp.setValid();
    return tmp;
}
