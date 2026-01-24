#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"
#include "../map/RoomHandle.h"
#include "../map/localspace.h"

#include <glm/glm.hpp>

#include <optional>

struct NODISCARD RoomRenderTransform final
{
    glm::vec3 roomCenter{};
    glm::vec3 renderCenter{};
    glm::vec3 localspaceOrigin{};
    float roomScale = 1.f;
    float localspaceScale = 1.f;
    std::optional<LocalSpaceId> localspaceId;
};

NODISCARD RoomRenderTransform getRoomRenderTransform(const RoomHandle &room);
NODISCARD glm::vec3 applyLocalspaceTransform(const RoomRenderTransform &transform,
                                             const glm::vec3 &pos);
NODISCARD glm::vec3 applyRoomGeometryTransform(const RoomRenderTransform &transform,
                                               const glm::vec3 &pos);
NODISCARD float getCombinedRoomScale(const RoomRenderTransform &transform);
