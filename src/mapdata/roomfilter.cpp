// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Elval' <ethorondil@gmail.com> (Elval)

#include "roomfilter.h"

#include "../global/Charset.h"
#include "../global/StringView.h"
#include "../global/TaggedString.h"
#include "../global/parserutils.h"
#include "../global/utils.h"
#include "../map/ExitFieldVariant.h"
#include "../map/enums.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../parser/Abbrev.h"
#include "../parser/AbstractParser-Commands.h"

#include <array>
#include <optional>
#include <regex>
#include <vector>

NODISCARD static std::regex createRegex(const std::string &input,
                                        const Qt::CaseSensitivity cs,
                                        const bool regex)
{
    // TODO: Switch from std::regex::extended to std::regex::multiline once GCC supports it
    auto options = std::regex::nosubs | std::regex::optimize | std::regex::extended;
    if (cs == Qt::CaseInsensitive) {
        options |= std::regex_constants::icase;
    }
    const std::string pattern = [&input, &options, &regex]() -> std::string {
        if (input.empty()) {
            return R"(^$)";
        }

        if (regex) {
            return input;
        }

        const auto escape_pattern = [&options]() {
            // Prevent user input from being interpreted as a POSIX extended regex
            if (options & std::regex::extended) {
                return R"([.[{}()*+?|^$])";
            }
            // ECMAScript escape pattern (unused)
            return R"([-[\]{}()*+?.,\^$|#\s])";
        };
        static const std::regex escape(escape_pattern(), std::regex::optimize);
        const std::string sanitized = std::regex_replace(input, escape, R"(\$&)");
        return ".*" + sanitized + ".*";
    }();
    return std::regex(pattern, options);
}

RoomFilter::RoomFilter(const std::string_view sv,
                       const Qt::CaseSensitivity cs,
                       const bool regex,
                       const PatternKindsEnum kind)
    : m_regex(createRegex(charset::conversion::utf8ToAscii(sv), cs, regex))
    , m_kind(kind)
{}

const char *const RoomFilter::parse_help
    = "Parse error; format is: [-(name|desc|contents|note|exits|all|clear)] pattern\n";

std::optional<RoomFilter> RoomFilter::parseRoomFilter(const std::string_view line)
{
    // REVISIT: rewrite this using the new syntax tree model.
    auto view = StringView{line}.trim();
    if (view.isEmpty()) {
        return std::nullopt;
    } else if (view.takeFirstLetter() != char_consts::C_MINUS_SIGN) {
        return RoomFilter{line, Qt::CaseInsensitive, false, PatternKindsEnum::NAME};
    }

    const auto first = view.takeFirstWord();
    const auto opt = [&first]() -> std::optional<PatternKindsEnum> {
        if (Abbrev("desc", 1).matches(first)) {
            return PatternKindsEnum::DESC;
        } else if (Abbrev("contents", 2).matches(first)) {
            return PatternKindsEnum::CONTENTS;
        } else if (Abbrev("name", 2).matches(first)) {
            return PatternKindsEnum::NAME;
        } else if (Abbrev("exits", 1).matches(first)) {
            return PatternKindsEnum::EXITS;
        } else if (Abbrev("note", 1).matches(first)) {
            return PatternKindsEnum::NOTE;
        } else if (Abbrev("all", 1).matches(first)) {
            return PatternKindsEnum::ALL;
        } else if (Abbrev("clear", 1).matches(first)) {
            return PatternKindsEnum::NONE;
        } else if (Abbrev("flags", 1).matches(first)) {
            return PatternKindsEnum::FLAGS;
        } else {
            return std::nullopt;
        }
    }();
    if (!opt.has_value()) {
        return std::nullopt;
    }

    const auto kind = opt.value();
    if (kind != PatternKindsEnum::NONE) {
        // Require pattern text in addition to arguments
        if (view.empty()) {
            return std::nullopt;
        }
    }
    return RoomFilter{view.toStdString(), Qt::CaseInsensitive, false, kind};
}

bool RoomFilter::filter_kind(const RawRoom &r, const PatternKindsEnum pat) const
{
    switch (pat) {
    case PatternKindsEnum::ALL:
        break;

    case PatternKindsEnum::DESC:
        return matches(r.getDescription());

    case PatternKindsEnum::CONTENTS:
        return matches(r.getContents());

    case PatternKindsEnum::NAME:
        return matches(r.getName());

    case PatternKindsEnum::NOTE:
        return matches(r.getNote());

    case PatternKindsEnum::EXITS:
        for (const auto &e : r.getExits()) {
            if (matches(e.getDoorName())) {
                return true;
            }
        }
        return false;

    case PatternKindsEnum::FLAGS: {
        for (const auto &e : r.getExits()) {
            if (this->matchesAny(e.getDoorFlags()) //
                || this->matchesAny(e.getExitFlags())) {
                return true;
            }
        }

        return this->matchesAny(r.getMobFlags())            //
               || this->matchesAny(r.getLoadFlags())        //
               || this->matchesDefined(r.getLightType())    //
               || this->matchesDefined(r.getSundeathType()) //
               || this->matchesDefined(r.getPortableType()) //
               || this->matchesDefined(r.getRidableType())  //
               || this->matchesDefined(r.getAlignType());
    }

    case PatternKindsEnum::NONE:
        return false;
    }

    throw std::invalid_argument("pat");
};

bool RoomFilter::filter(const RawRoom &r) const
{
    if (m_kind != PatternKindsEnum::ALL) {
        return filter_kind(r, m_kind);
    }

    // NOTE: using C-style array allows static assert on the number of elements, but std::array doesn't
    // in this case because the compiler will report excess elements but not too few elements.
    // Alternate: make this a std::vector and then either do a regular assert, or remove the assert.
    static constexpr const PatternKindsEnum ALL_KINDS[]{PatternKindsEnum::DESC,
                                                        PatternKindsEnum::CONTENTS,
                                                        PatternKindsEnum::NAME,
                                                        PatternKindsEnum::NOTE,
                                                        PatternKindsEnum::EXITS,
                                                        PatternKindsEnum::FLAGS};
    static constexpr const size_t ALL_KINDS_SIZE = sizeof(ALL_KINDS) / sizeof(ALL_KINDS[0]);
    static_assert(ALL_KINDS_SIZE == PATTERN_KINDS_LENGTH - 2); // excludes NONE and ALL.
    for (const auto &pat : ALL_KINDS) {
        if (filter_kind(r, pat)) {
            return true;
        }
    }
    return false;
}
