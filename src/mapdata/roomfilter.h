#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Ethorondil' <ethorondil@gmail.com> (Elval)

#include <cassert>
#include <optional>
#include <regex>
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
    explicit RoomFilter(QString str, const Qt::CaseSensitivity cs, const PatternKindsEnum kind);

public:
    bool filter(const Room *r) const;
    PatternKindsEnum patternKind() const { return kind; }

private:
    const std::regex regex;
    const PatternKindsEnum kind;
};
