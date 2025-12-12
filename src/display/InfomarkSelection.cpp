// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "InfomarkSelection.h"

#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../mapdata/mapdata.h"

#include <algorithm>
#include <cassert>
#include <vector>

// Assumes arguments are pre-scaled by INFOMARK_SCALE;
// TODO: add a new type to avoid accidental conversion
// from "world scale" Coordinate to "infomark scale" Coordinate.
InfomarkSelection::InfomarkSelection(Badge<InfomarkSelection>,
                                     MapData &mapData,
                                     const Coordinate &c1,
                                     const Coordinate &c2)
    : m_mapData{mapData}
    , m_sel1{c1}
    , m_sel2{c2}
{}

void InfomarkSelection::init()
{
    const auto &c1 = m_sel1;
    const auto &c2 = m_sel2;

    const auto z = c1.z;

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

    const auto isMarkerInSelection = [z, &isCoordInSelection](const InfomarkHandle &marker) -> bool {
        const Coordinate &pos1 = marker.getPosition1();
        if (z != pos1.z) {
            return false;
        }

        if (isCoordInSelection(pos1)) {
            return true;
        }

        if (marker.getType() == InfomarkTypeEnum::TEXT) {
            return false;
        }

        const Coordinate &pos2 = marker.getPosition2();
        if (z != pos2.z) {
            return false;
        }

        return isCoordInSelection(pos2);
    };

    const auto &map = m_mapData.getCurrentMap();
    const auto &db = map.getInfomarkDb();
    for (const InfomarkId id : db.getIdSet()) {
        if (isMarkerInSelection(InfomarkHandle{db, id})) {
            m_markerList.emplace_back(id);
        }
    }
}

void InfomarkSelection::applyOffset(const Coordinate &offset) const
{
    if (m_markerList.empty()) {
        qWarning() << "No markers selected.";
        return;
    }

    ChangeList changes;
    const auto &map = m_mapData.getCurrentMap();
    const auto &db = map.getInfomarkDb();
    for (const InfomarkId id : m_markerList) {
        try {
            RawInfomark fields = db.getRawCopy(id);
            fields.offsetBy(offset);
            changes.add(Change{infomark_change_types::UpdateInfomark{id, fields}});
        } catch (const std::out_of_range &e) {
            qWarning() << "failed to find marker" << id.value() << ":" << e.what();
            assert(false);
        }
    }
    if (changes.empty()) {
        return;
    }

    const auto count = changes.getChanges().size();
    if (m_mapData.applyChanges(changes)) {
        qInfo() << "Applied offset to" << count << "marker(s).";
    } else {
        qWarning() << "Failed to apply offset to" << count << "marker(s).";
    }
}
