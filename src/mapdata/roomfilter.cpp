/************************************************************************
**
** Authors:   ethorondil
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "roomfilter.h"

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/utils.h"
#include "ExitFieldVariant.h"
#include "mmapper2room.h"

const char *RoomFilter::parse_help
    = "Parse error; format is: [-(name|desc|dyndesc|note|exits|all)] pattern\r\n";

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
            kind = pattern_kinds::ALL_BUT_EXITS;
        } else {
            return false;
        }

        pattern = line.section(" ", 1);
        if (pattern.isEmpty()) {
            return false;
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
        case pattern_kinds::ALL_BUT_EXITS:
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

        case pattern_kinds::NONE:
            return false;
        }
        return false;
    };

    if (kind == pattern_kinds::ALL_BUT_EXITS) {
        for (auto pat : {pattern_kinds::DESC,
                         pattern_kinds::DYN_DESC,
                         pattern_kinds::NAME,
                         pattern_kinds::NOTE})
            if (filter_kind(r, pat))
                return true;
        return false;
    } else {
        return filter_kind(r, this->kind);
    }
}
