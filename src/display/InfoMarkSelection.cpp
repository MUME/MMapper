// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "InfoMarkSelection.h"

#include <algorithm>
#include <cassert>

#include "../expandoracommon/coordinate.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"

InfoMarkSelection::InfoMarkSelection(this_is_private,
                                     MapData *const mapData,
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

    assert(mapData);
    for (const auto &marker : mapData->getMarkersList()) {
        const Coordinate pos1 = marker->getPosition1();
        const Coordinate pos2 = marker->getPosition2();

        bool firstInside = false;
        bool secondInside = false;

        if (pos1.x > bx1 && pos1.x < bx2 && pos1.y > by1 && pos1.y < by2) {
            firstInside = true;
        }
        if (pos2.x > bx1 && pos2.x < bx2 && pos2.y > by1 && pos2.y < by2) {
            secondInside = true;
        }
        if (c1.z == pos1.z && c2.z == pos2.z && (firstInside || secondInside)) {
            emplace_back(marker);
        }
    }
}
