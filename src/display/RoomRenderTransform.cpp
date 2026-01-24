// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "RoomRenderTransform.h"

#include "../map/World.h"

#include <cmath>

NODISCARD static float sanitizeRoomScale(const float scale)
{
    if (!std::isfinite(scale) || scale <= 0.f) {
        return 1.f;
    }
    return scale;
}

RoomRenderTransform getRoomRenderTransform(const RoomHandle &room)
{
    RoomRenderTransform transform;
    const auto &pos = room.getPosition();
    transform.roomCenter = pos.to_vec3() + glm::vec3{0.5f, 0.5f, 0.f};
    transform.roomScale = sanitizeRoomScale(room.getScaleFactor());

    const Map map = room.getMap();
    const auto &world = map.getWorld();
    transform.localspaceId = world.getRoomLocalSpace(room.getId());

    if (transform.localspaceId) {
        if (const auto renderData = world.getLocalSpaceRenderData(*transform.localspaceId)) {
            transform.localspaceScale = renderData->portalScale;
            transform.localspaceOrigin = glm::vec3{renderData->portalX,
                                                   renderData->portalY,
                                                   renderData->portalZ}
                                         - glm::vec3{renderData->localCx,
                                                     renderData->localCy,
                                                     renderData->localCz}
                                               * transform.localspaceScale;
        }
    }

    transform.renderCenter = applyLocalspaceTransform(transform, transform.roomCenter);
    return transform;
}

glm::vec3 applyLocalspaceTransform(const RoomRenderTransform &transform, const glm::vec3 &pos)
{
    return transform.localspaceOrigin + pos * transform.localspaceScale;
}

glm::vec3 applyRoomGeometryTransform(const RoomRenderTransform &transform, const glm::vec3 &pos)
{
    const glm::vec3 scaledPos
        = transform.roomCenter + (pos - transform.roomCenter) * transform.roomScale;
    return applyLocalspaceTransform(transform, scaledPos);
}

float getCombinedRoomScale(const RoomRenderTransform &transform)
{
    return transform.roomScale * transform.localspaceScale;
}
