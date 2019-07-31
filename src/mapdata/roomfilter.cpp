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
    auto kind = PatternKindsEnum::NAME;
    if (line.size() >= 2 && line[0] == '-') {
        QString kindstr = line.section(" ", 0, 0);
        if (kindstr == "-desc" || kindstr.startsWith("-d")) {
            kind = PatternKindsEnum::DESC;
        } else if (kindstr == "-dyndesc" || kindstr.startsWith("-dy")) {
            kind = PatternKindsEnum::DYN_DESC;
        } else if (kindstr == "-name") {
            kind = PatternKindsEnum::NAME;
        } else if (kindstr == "-exits" || kindstr == "-e") {
            kind = PatternKindsEnum::EXITS;
        } else if (kindstr == "-note" || kindstr == "-n") {
            kind = PatternKindsEnum::NOTE;
        } else if (kindstr == "-all" || kindstr == "-a") {
            kind = PatternKindsEnum::ALL;
        } else if (kindstr == "-clear" || kindstr == "-c") {
            kind = PatternKindsEnum::NONE;
        } else if (kindstr == "-flags" || kindstr == "-f") {
            kind = PatternKindsEnum::FLAGS;
        } else {
            return false;
        }

        // Require pattern text in addition to arguments
        if (kind != PatternKindsEnum::NONE) {
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

    const auto filter_kind = [this](const Room &r, const PatternKindsEnum pat) -> bool {
        switch (pat) {
        case PatternKindsEnum::ALL:
            throw std::invalid_argument("pat");

        case PatternKindsEnum::DESC:
            return r.getStaticDescription().toQString().contains(pattern, cs);

        case PatternKindsEnum::DYN_DESC:
            return r.getDynamicDescription().toQString().contains(pattern, cs);

        case PatternKindsEnum::NAME:
            return r.getName().toQString().contains(pattern, cs);

        case PatternKindsEnum::NOTE:
            return r.getNote().toQString().contains(pattern, cs);

        case PatternKindsEnum::EXITS:
            for (const auto &e : r.getExitsList()) {
                if (e.getDoorName().toQString().contains(pattern, cs)) {
                    return true;
                }
            }
            return false;

        case PatternKindsEnum::FLAGS:
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

        case PatternKindsEnum::NONE:
            return false;
        }
        return false;
    };

    if (kind == PatternKindsEnum::ALL) {
        for (auto pat : {PatternKindsEnum::DESC,
                         PatternKindsEnum::DYN_DESC,
                         PatternKindsEnum::NAME,
                         PatternKindsEnum::NOTE,
                         PatternKindsEnum::EXITS,
                         PatternKindsEnum::FLAGS})
            if (filter_kind(r, pat))
                return true;
        return false;
    } else {
        return filter_kind(r, this->kind);
    }
}
