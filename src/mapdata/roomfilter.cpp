// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: 'Ethorondil' <ethorondil@gmail.com> (Elval)

#include "roomfilter.h"

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/utils.h"
#include "../parser/AbstractParser-Commands.h"
#include "ExitFieldVariant.h"
#include "enums.h"
#include "mmapper2room.h"

const char *RoomFilter::parse_help
    = "Parse error; format is: [-(name|desc|dyndesc|note|exits|all|clear)] pattern\r\n";

bool RoomFilter::parseRoomFilter(const QString &line, RoomFilter &output)
{
    QString pattern = line;
    auto kind = pattern_kinds::NAME;
    if (line.size() >= 2 && line[0] == '-') {
        QString kindstr = line.section(" ", 0, 0);
        if (kindstr == "-desc" || kindstr.startsWith("-d")) {
            kind = pattern_kinds::DESC;
        } else if (kindstr == "-dyndesc" || kindstr.startsWith("-dy")) {
            kind = pattern_kinds::DYN_DESC;
        } else if (kindstr == "-name") {
            kind = pattern_kinds::NAME;
        } else if (kindstr == "-exits" || kindstr == "-e") {
            kind = pattern_kinds::EXITS;
        } else if (kindstr == "-note" || kindstr == "-n") {
            kind = pattern_kinds::NOTE;
        } else if (kindstr == "-all" || kindstr == "-a") {
            kind = pattern_kinds::ALL;
        } else if (kindstr == "-clear" || kindstr == "-c") {
            kind = pattern_kinds::NONE;
        } else if (kindstr == "-flags" || kindstr == "-f") {
            kind = pattern_kinds::FLAGS;
        } else {
            return false;
        }

        // Require pattern text in addition to arguments
        if (kind != pattern_kinds::NONE) {
            pattern = line.section(" ", 1);
            if (pattern.isEmpty()) {
                return false;
            }
        }
    }
    output = RoomFilter(pattern, Qt::CaseInsensitive, kind);
    return true;
}

bool RoomFilter::filter(const Room *const pr) const
{
    auto &r = deref(pr);

    const auto filter_kind = [this](const Room &r, const pattern_kinds pat) -> bool {
        switch (pat) {
        case pattern_kinds::ALL:
            throw std::invalid_argument("pat");

        case pattern_kinds::DESC:
            return r.getStaticDescription().contains(pattern, cs);

        case pattern_kinds::DYN_DESC:
            return r.getDynamicDescription().contains(pattern, cs);

        case pattern_kinds::NAME:
            return r.getName().contains(pattern, cs);

        case pattern_kinds::NOTE:
            return r.getNote().contains(pattern, cs);

        case pattern_kinds::EXITS:
            for (const auto &e : r.getExitsList()) {
                if (e.getDoorName().contains(pattern, cs)) {
                    return true;
                }
            }
            return false;

        case pattern_kinds::FLAGS:
            for (const auto &e : r.getExitsList()) {
                for (const auto flag : ALL_DOOR_FLAGS) {
                    if (!e.getDoorFlags().contains(flag))
                        continue;
                    if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                        return true;
                }
                for (const auto flag : ALL_EXIT_FLAGS) {
                    if (!e.getExitFlags().contains(flag))
                        continue;
                    if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                        return true;
                }
            }
            for (const auto flag : ALL_MOB_FLAGS) {
                if (!r.getMobFlags().contains(flag))
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            for (const auto flag : ALL_LOAD_FLAGS) {
                if (!r.getLoadFlags().contains(flag))
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            for (const auto flag : DEFINED_ROOM_LIGHT_TYPES) {
                if (r.getLightType() != flag)
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            for (const auto flag : DEFINED_ROOM_SUNDEATH_TYPES) {
                if (r.getSundeathType() != flag)
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            for (const auto flag : DEFINED_ROOM_PORTABLE_TYPES) {
                if (r.getPortableType() != flag)
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            for (const auto flag : DEFINED_ROOM_RIDABLE_TYPES) {
                if (r.getRidableType() != flag)
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            for (const auto flag : DEFINED_ROOM_ALIGN_TYPES) {
                if (r.getAlignType() != flag)
                    continue;
                if (QString(getParserCommandName(flag).getCommand()).contains(pattern, cs))
                    return true;
            }
            return false;

        case pattern_kinds::NONE:
            return false;
        }
        return false;
    };

    if (kind == pattern_kinds::ALL) {
        for (auto pat : {pattern_kinds::DESC,
                         pattern_kinds::DYN_DESC,
                         pattern_kinds::NAME,
                         pattern_kinds::NOTE,
                         pattern_kinds::EXITS,
                         pattern_kinds::FLAGS})
            if (filter_kind(r, pat))
                return true;
        return false;
    } else {
        return filter_kind(r, this->kind);
    }
}
