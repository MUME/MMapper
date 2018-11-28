/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#include "InfoMarkSelection.h"

#include <algorithm>
#include <cassert>

#include "../expandoracommon/coordinate.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"

InfoMarkSelection::InfoMarkSelection(MapData *const mapData,
                                     const Coordinate &c1,
                                     const Coordinate &c2,
                                     const int margin)
{
    m_sel1 = c1;
    m_sel2 = c2;

    const auto bx1 = std::min(c1.x, c2.x) - margin;
    const auto by1 = std::min(c1.y, c2.y) - margin;
    const auto bx2 = std::max(c1.x, c2.x) + margin;
    const auto by2 = std::max(c1.y, c2.y) + margin;

    bool firstInside = false;
    bool secondInside = false;

    assert(mapData);
    for (const auto marker : mapData->getMarkersList()) {
        const Coordinate pos1 = marker->getPosition1();
        const Coordinate pos2 = marker->getPosition2();

        firstInside = false;
        secondInside = false;

        if (pos1.x > bx1 && pos1.x < bx2 && pos1.y > by1 && pos1.y < by2) {
            firstInside = true;
        }
        if (pos2.x > bx1 && pos2.x < bx2 && pos2.y > by1 && pos2.y < by2) {
            secondInside = true;
        }
        if (c1.z == pos1.z && c2.z == pos2.z && (firstInside || secondInside)) {
            append(marker);
        }
    }
}

InfoMarkSelection::InfoMarkSelection(MapData *const mapData, const Coordinate &c)
    : InfoMarkSelection(mapData, c, c, 100)
{
    // REVISIT: What margin should we pick?
}
