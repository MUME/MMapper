#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Elval' <ethorondil@gmail.com> (Elval)

#include "../map/RoomHandle.h"
#include "../map/room.h"

#include <cassert>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>

#include <QString>
#include <QtCore>

enum class NODISCARD PatternKindsEnum { NONE, DESC, CONTENTS, NAME, NOTE, EXITS, FLAGS, ALL };
static constexpr const auto PATTERN_KINDS_LENGTH = static_cast<size_t>(PatternKindsEnum::ALL) + 1u;
static_assert(PATTERN_KINDS_LENGTH == 8);

class NODISCARD RoomFilter final
{
public:
    static const char *const parse_help;
    NODISCARD static std::optional<RoomFilter> parseRoomFilter(std::string_view line);

private:
    const std::regex m_regex;
    const PatternKindsEnum m_kind;

public:
    RoomFilter() = delete;
    explicit RoomFilter(std::string_view str,
                        Qt::CaseSensitivity cs,
                        bool regex,
                        PatternKindsEnum kind);

public:
    NODISCARD bool filter(const RawRoom &r) const;
    NODISCARD PatternKindsEnum patternKind() const { return m_kind; }

private:
    NODISCARD bool filter_kind(const RawRoom &r, const PatternKindsEnum pat) const;
    NODISCARD bool matches(const std::string_view s) const
    {
        return std::regex_match(s.begin(), s.end(), m_regex);
    }

private:
    template<typename T>
    NODISCARD bool matches(const TaggedStringUtf8<T> &s) const
    {
        return this->matches(s.getStdStringUtf8());
    }
    template<typename T>
    NODISCARD bool matches(const TaggedBoxedStringUtf8<T> &s) const
    {
        return this->matches(s.getStdStringViewUtf8());
    }

private:
    template<typename E>
    NODISCARD bool matchesParserCommand(const E type) const
    {
        static_assert(std::is_enum_v<E>);
        const auto s = getParserCommandName(type).getCommand();
        assert(s != nullptr);
        return s != nullptr && this->matches(s);
    }

private:
    template<typename F> // enum::Flags<...>
    NODISCARD bool matchesAny(const F &flags) const
    {
        auto pred = [this](auto flag) -> bool { return this->matchesParserCommand(flag); };
        return flags.find_first_matching(pred).has_value();
    }

private:
    /// Note: Assumes enum E has value E::UNDEFINED.
    template<typename E>
    NODISCARD bool matchesDefined(const E type) const
    {
        return (type != E::UNDEFINED) && this->matchesParserCommand(type);
    }
};
