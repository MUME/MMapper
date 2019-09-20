// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "InfoMarkSelection.h"

#include <algorithm>
#include <cassert>

#include "../expandoracommon/coordinate.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"

// Assumes arguments are pre-scaled by INFOMARK_SCALE;
// TODO: add a new type to avoid accidental conversion
// from "world scale" Coordinate to "infomark scale" Coordinate.
InfoMarkSelection::InfoMarkSelection(this_is_private,
                                     MapData *const mapData,
                                     const Coordinate &c1,
                                     const Coordinate &c2)
{
    const auto z = c1.z;

    m_sel1 = c1;
    m_sel2 = c2;

    if (c2.z != z) {
        assert(false);
        m_sel2.z = z;
    }

    const auto bx1 = std::min(c1.x, c2.x);
    const auto by1 = std::min(c1.y, c2.y);
    const auto bx2 = std::max(c1.x, c2.x);
    const auto by2 = std::max(c1.y, c2.y);

    const auto isCoordInSelection = [bx1, by1, bx2, by2](const Coordinate &c) -> bool {
        return isClamped(c.x, bx1, bx2) && isClamped(c.y, by1, by2);
    };

    const auto isMarkerInSelection = [z, &isCoordInSelection](const auto &marker) -> bool {
        const Coordinate &pos1 = marker->getPosition1();
        if (z != pos1.z)
            return false;

        if (isCoordInSelection(pos1))
            return true;

        if (marker->getType() == InfoMarkTypeEnum::TEXT)
            return false;

        const Coordinate &pos2 = marker->getPosition2();
        if (z != pos2.z)
            return false;

        return isCoordInSelection(pos2);
    };

    assert(mapData);
    for (const auto &marker : mapData->getMarkersList()) {
        if (isMarkerInSelection(marker)) {
            emplace_back(marker);
        }
    }
}
