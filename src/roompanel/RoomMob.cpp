// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RoomMob.h"

#include "../parser/abstractparser.h"

#include <memory>
#include <utility>

#include <QDebug>
#include <QMessageLogContext>

// ----------------------------- RoomMob --------------------------------------
SharedRoomMob RoomMob::alloc()
{
    return std::make_shared<RoomMob>(Badge<RoomMob>{});
}

RoomMob::RoomMob(Badge<RoomMob>) {}

RoomMob::~RoomMob() = default;

bool RoomMob::updateFrom(RoomMobUpdate &&data)
{
    if (getId() != data.getId()) {
        qWarning() << "Ignoring RoomMob id=" << getId()
                   << " update request with different id=" << data.getId();
        return false;
    }
    bool updated = false;
    for (size_t i = 0; i < static_cast<size_t>(NUM_MOB_FIELDS); ++i) {
        const auto f = static_cast<Field>(i);
        if (data.contains(f) && getField(f) != data.getField(f)) {
            setField(f, data.getField(f));
            updated = true;
        }
    }
    return updated;
}
