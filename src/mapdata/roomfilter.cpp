// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Ethorondil' <ethorondil@gmail.com> (Elval)

#include "roomfilter.h"

#include <optional>
#include <regex>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/StringView.h"
#include "../global/TaggedString.h"
#include "../global/utils.h"
#include "../parser/AbstractParser-Commands.h"
#include "../parser/parserutils.h"
#include "ExitFieldVariant.h"
#include "enums.h"
#include "mmapper2room.h"

static std::regex createRegex(const std::string &input, const Qt::CaseSensitivity cs)
{
    // REVISIT: Add another option to support regex where we do not santize the input
    static const std::regex escape(R"([-[\]{}()*+?.,\^$|#])", std::regex::optimize);
    const std::string sanitized = std::regex_replace(input, escape, R"(\$&)");
    const std::string pattern = ".*" + sanitized + ".*";
    auto options = std::regex::nosubs | std::regex::optimize | std::regex::extended;
    if (cs == Qt::CaseInsensitive)
        options |= std::regex_constants::icase;
    return std::regex(pattern, options);
}

RoomFilter::RoomFilter(QString str, const Qt::CaseSensitivity cs, const PatternKindsEnum kind)
    : m_regex(createRegex(ParserUtils::latinToAsciiInPlace(str).toStdString(), cs))
    , m_kind(kind)
{}

const char *const RoomFilter::parse_help
    = "Parse error; format is: [-(name|desc|dyndesc|note|exits|all|clear)] pattern\r\n";

std::optional<RoomFilter> RoomFilter::parseRoomFilter(const QString &line)
{
    // REVISIT: rewrite this using the new syntax tree model.
    std::string s = line.toLatin1().toStdString();
    auto view = StringView{s}.trim();
    if (view.isEmpty())
        return std::nullopt;

    auto kind = PatternKindsEnum::NAME;
    QString pattern = line;
    if (view.takeFirstLetter() == '-') {
        const auto sub = view.takeFirstWord();
        const auto &subStr = sub.toStdString();
        if (subStr == "desc" || sub.startsWith("de") || subStr == "d") {
            kind = PatternKindsEnum::DESC;
        } else if (subStr == "dyndesc" || sub.startsWith("dy") || subStr == "y") {
            kind = PatternKindsEnum::DYN_DESC;
        } else if (subStr == "name") {
            kind = PatternKindsEnum::NAME;
        } else if (subStr == "exits" || subStr == "e") {
            kind = PatternKindsEnum::EXITS;
        } else if (subStr == "note" || subStr == "n") {
            kind = PatternKindsEnum::NOTE;
        } else if (subStr == "all" || subStr == "a") {
            kind = PatternKindsEnum::ALL;
        } else if (subStr == "clear" || subStr == "c") {
            kind = PatternKindsEnum::NONE;
        } else if (subStr == "flags" || subStr == "f") {
            kind = PatternKindsEnum::FLAGS;
        } else {
            return std::nullopt;
        }

        // Require pattern text in addition to arguments
        if (kind != PatternKindsEnum::NONE) {
            pattern = view.toQString();
            if (pattern.isEmpty()) {
                return std::nullopt;
            }
        }
    }
    return RoomFilter{pattern, Qt::CaseInsensitive, kind};
}

bool RoomFilter::filter(const Room *const pr) const
{
    auto &r = deref(pr);
    const auto filter_kind = [this](const Room &r, const PatternKindsEnum pat) -> bool {
        switch (pat) {
        case PatternKindsEnum::ALL:
            break;

        case PatternKindsEnum::DESC:
            return matches(r.getStaticDescription());

        case PatternKindsEnum::DYN_DESC:
            return matches(r.getDynamicDescription());

        case PatternKindsEnum::NAME:
            return matches(r.getName());

        case PatternKindsEnum::NOTE:
            return matches(r.getNote());

        case PatternKindsEnum::EXITS:
            for (const auto &e : r.getExitsList()) {
                if (matches(e.getDoorName())) {
                    return true;
                }
            }
            return false;

        case PatternKindsEnum::FLAGS: {
            for (const auto &e : r.getExitsList()) {
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

    if (m_kind != PatternKindsEnum::ALL) {
        return filter_kind(r, m_kind);
    }

    // Note: This is implicitly a std::initializer_list<PatternKindsEnum>.
    static constexpr const auto ALL_KINDS = {PatternKindsEnum::DESC,
                                             PatternKindsEnum::DYN_DESC,
                                             PatternKindsEnum::NAME,
                                             PatternKindsEnum::NOTE,
                                             PatternKindsEnum::EXITS,
                                             PatternKindsEnum::FLAGS};
    static_assert(ALL_KINDS.size() == PATTERN_KINDS_LENGTH - 2); // excludes NONE and ALL.
    for (const auto &pat : ALL_KINDS) {
        if (filter_kind(r, pat)) {
            return true;
        }
    }
    return false;
}
