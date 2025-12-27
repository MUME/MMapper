#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Color.h"
#include "RuleOf5.h"
#include "utils.h"

#include <string_view>
#include <vector>

#undef TRANSPARENT // Bad dog, Microsoft; bad dog!!!

// X(_id, _name)
#define XFOREACH_NAMED_COLOR_OPTIONS(X) \
    X(BACKGROUND, "background") \
    X(CONNECTION_NORMAL, "connection-normal") \
    X(HIGHLIGHT_NEEDS_SERVER_ID, "highlight-needs-server-id") \
    X(HIGHLIGHT_UNSAVED, "highlight-unsaved") \
    X(HIGHLIGHT_TEMPORARY, "highlight-temporary") \
    X(INFOMARK_COMMENT, "infomark-comment") \
    X(INFOMARK_HERB, "infomark-herb") \
    X(INFOMARK_MOB, "infomark-mob") \
    X(INFOMARK_OBJECT, "infomark-object") \
    X(INFOMARK_RIVER, "infomark-river") \
    X(INFOMARK_ROAD, "infomark-road") \
    X(ROOM_DARK, "room-dark") \
    X(ROOM_NO_SUNDEATH, "room-no-sundeath") \
    X(STREAM, "stream") \
    X(TRANSPARENT, ".transparent") \
    X(VERTICAL_COLOR_CLIMB, "vertical-climb") \
    X(VERTICAL_COLOR_REGULAR_EXIT, "vertical-regular") \
    X(WALL_COLOR_BUG_WALL_DOOR, "wall-bug-wall-door") \
    X(WALL_COLOR_CLIMB, "wall-climb") \
    X(WALL_COLOR_FALL_DAMAGE, "wall-fall-damage") \
    X(WALL_COLOR_GUARDED, "wall-guarded") \
    X(WALL_COLOR_NO_FLEE, "wall-no-flee") \
    X(WALL_COLOR_NO_MATCH, "wall-no-match") \
    X(WALL_COLOR_NOT_MAPPED, "wall-not-mapped") \
    X(WALL_COLOR_RANDOM, "wall-random") \
    X(WALL_COLOR_REGULAR_EXIT, "wall-regular-exit") \
    X(WALL_COLOR_SPECIAL, "wall-special")

#define X_DECL(_id, _name) _id,
enum class NODISCARD NamedColorEnum : uint8_t { DEFAULT = 0, XFOREACH_NAMED_COLOR_OPTIONS(X_DECL) };
#undef X_DECL
#define X_COUNT(_id, _name) +1
static inline constexpr size_t NUM_NAMED_COLORS = XFOREACH_NAMED_COLOR_OPTIONS(X_COUNT) + 1;
#undef X_COUNT

// TODO: rename this, but to what? NamedColorHandle?
class NODISCARD XNamedColor final
{
private:
    NamedColorEnum m_value = NamedColorEnum::DEFAULT;

public:
    NODISCARD static std::optional<XNamedColor> lookup(std::string_view sv);

public:
    XNamedColor() = default;
    XNamedColor(std::nullptr_t) = delete;
    explicit XNamedColor(const NamedColorEnum color)
        : m_value{color}
    {
        assert(getIndex() < NUM_NAMED_COLORS);
    }

public:
    ~XNamedColor();

public:
    DEFAULT_CTORS(XNamedColor);
    DELETE_ASSIGN_OPS(XNamedColor);

public:
    NODISCARD bool isInitialized() const;
    NODISCARD size_t getIndex() const noexcept { return static_cast<uint8_t>(m_value); }
    NODISCARD NamedColorEnum getNamedColorEnum() const noexcept { return m_value; }

public:
    NODISCARD const std::string &getName() const;
    NODISCARD Color getColor() const;
    // note: you can't actually set DEFAULT or TRANSPARENT
    ALLOW_DISCARD bool setColor(Color c);

public:
    NODISCARD explicit operator Color() const { return getColor(); }
    XNamedColor &operator=(Color c)
    {
        setColor(c);
        return *this;
    }

public:
    NODISCARD bool operator==(const XNamedColor &rhs) const { return m_value == rhs.m_value; }
    NODISCARD bool operator!=(const XNamedColor &rhs) const { return !(rhs == *this); }

public:
    NODISCARD static const std::vector<Color> &getAllColors();
    NODISCARD static const std::vector<std::string> &getAllNames();
};
