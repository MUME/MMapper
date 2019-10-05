#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Ethorondil' <ethorondil@gmail.com> (Elval)

#include <cassert>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <QString>
#include <QtCore>

#include "../expandoracommon/room.h"

class Room;

enum class PatternKindsEnum { NONE, DESC, DYN_DESC, NAME, NOTE, EXITS, FLAGS, ALL };
static constexpr const auto PATTERN_KINDS_LENGTH = static_cast<size_t>(PatternKindsEnum::ALL) + 1u;
static_assert(PATTERN_KINDS_LENGTH == 8);

class RoomFilter final
{
public:
    static const char *const parse_help;
    static std::optional<RoomFilter> parseRoomFilter(const QString &line);

public:
    RoomFilter() = delete;
    explicit RoomFilter(QString str, Qt::CaseSensitivity cs, PatternKindsEnum kind);

public:
    bool filter(const Room *r) const;
    PatternKindsEnum patternKind() const { return m_kind; }

private:
    bool matches(const std::string_view &s) const
    {
        return std::regex_match(s.begin(), s.end(), m_regex);
    }

private:
    template<typename T>
    bool matches(const TaggedString<T> &s) const
    {
        return this->matches(s.getStdString());
    }

private:
    template<typename E>
    bool matchesParserCommand(const E type) const
    {
        static_assert(std::is_enum_v<E>);
        const auto s = getParserCommandName(type).getCommand();
        assert(s != nullptr);
        return s != nullptr && this->matches(s);
    }

private:
    template<typename F> // enum::Flags<...>
    bool matchesAny(const F &flags) const
    {
        auto pred = [this](auto flag) -> bool { return this->matchesParserCommand(flag); };
        return flags.find_first_matching(pred).has_value();
    }

private:
    /// Note: Assumes enum E has value E::UNDEFINED.
    template<typename E>
    bool matchesDefined(const E type) const
    {
        return (type != E::UNDEFINED) && this->matchesParserCommand(type);
    }

private:
    const std::regex m_regex;
    const PatternKindsEnum m_kind;
};
