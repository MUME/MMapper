// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "ProjectionUtils.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QMatrix4x4>

namespace ProjectionUtils {

float calculatePitchDegrees(const ViewportConfig &config, float zoomScale)
{
    if (!config.autoTilt) {
        return config.verticalAngleDegrees;
    }

    return glm::smoothstep(0.5f, 2.f, zoomScale) * config.verticalAngleDegrees;
}

glm::mat4 calculateViewProj(const ViewportConfig &config,
                            const glm::vec2 &scrollPos,
                            const glm::ivec2 &size,
                            const float zoomScale,
                            const int currentLayer)
{
    const int width = size.x;
    const int height = size.y;

    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    const float fovDegrees = config.fovDegrees;
    const auto pitchRadians = glm::radians(calculatePitchDegrees(config, zoomScale));
    const auto yawRadians = glm::radians(config.horizontalAngleDegrees);
    const auto layerHeight = config.layerHeight;

    const auto pixelScale = std::invoke([aspect, fovDegrees, width]() -> float {
        constexpr float HARDCODED_LOGICAL_PIXELS = 44.f;
        const auto dummyProj = glm::perspective(glm::radians(fovDegrees), aspect, 1.f, 10.f);

        const auto centerRoomProj = glm::inverse(dummyProj) * glm::vec4(0.f, 0.f, 0.f, 1.f);
        const auto centerRoom = glm::vec3(centerRoomProj) / centerRoomProj.w;

        // Use east instead of north, so that tilted perspective matches horizontally.
        const auto oneRoomEast = dummyProj * glm::vec4(centerRoom + glm::vec3(1.f, 0.f, 0.f), 1.f);
        const float clipDist = std::abs(oneRoomEast.x / oneRoomEast.w);
        const float ndcDist = clipDist * 0.5f;

        // width is in logical pixels
        const float screenDist = ndcDist * static_cast<float>(width);
        const auto pixels = std::abs(centerRoom.z) * screenDist;
        return pixels / HARDCODED_LOGICAL_PIXELS;
    });

    const float ZSCALE = layerHeight;
    const float camDistance = pixelScale / zoomScale;
    const auto center = glm::vec3(scrollPos, static_cast<float>(currentLayer) * ZSCALE);

    // The view matrix will transform from world space to eye-space.
    // Eye space has the camera at the origin, with +X right, +Y up, and -Z forward.
    //
    // Our camera's orientation is based on the world-space ENU coordinates.
    // We'll define right-handed basis vectors forward, right, and up.

    // The horizontal rotation in the XY plane will affect both forward and right vectors.
    // Currently the convention is: -45 is northwest, and +45 is northeast.
    //
    // If you want to modify this, keep in mind that the angle is inverted since the
    // camera is subtracted from the center, so the result is that positive angle
    // appears clockwise (backwards) on screen.
    const auto rotateHorizontal = glm::mat3(
        glm::rotate(glm::mat4(1), -yawRadians, glm::vec3(0, 0, 1)));

    // Our unrotated pitch is defined so that 0 is straight down, and 90 degrees is north,
    // but the yaw rotation can cause it to point northeast or northwest.
    //
    // Here we use an ENU coordinate system, so we have:
    //   forward(pitch= 0 degrees) = -Z (down), and
    //   forward(pitch=90 degrees) = +Y (north).
    const auto forward = rotateHorizontal
                         * glm::vec3(0.f, std::sin(pitchRadians), -std::cos(pitchRadians));
    // Unrotated right is east (+X).
    const auto right = rotateHorizontal * glm::vec3(1, 0, 0);
    // right x forward = up
    const auto up = glm::cross(right, glm::normalize(forward));

    // Subtract because camera looks at the center.
    const auto eye = center - camDistance * forward;

    // NOTE: may need to modify near and far planes by pixelScale and zoomScale.
    // Be aware that a 24-bit depth buffer only gives about 12 bits of usable
    // depth range; we may need to reduce this for people with 16-bit depth buffers.
    // Keep in mind: Arda is about 600x300 rooms, so viewing the blue mountains
    // from mordor requires approx 700 room units of view distance.
    const auto proj = glm::perspective(glm::radians(fovDegrees), aspect, 0.25f, 1024.f);
    const auto view = glm::lookAt(eye, center, up);
    const auto scaleZ = glm::scale(glm::mat4(1), glm::vec3(1.f, 1.f, ZSCALE));

    return proj * view * scaleZ;
}

glm::mat4 calculateViewProjOld(const glm::vec2 &scrollPos,
                               const glm::ivec2 &size,
                               const float zoomScale,
                               const int /*currentLayer*/)
{
    constexpr float FIXED_VIEW_DISTANCE = 60.f;
    constexpr auto baseSize = static_cast<float>(ProjectionUtils::BASESIZE);

    const float swp = zoomScale * baseSize / static_cast<float>(size.x);
    const float shp = zoomScale * baseSize / static_cast<float>(size.y);

    QMatrix4x4 proj;
    proj.frustum(-0.5f, +0.5f, -0.5f, 0.5f, 5.f, 10000.f);
    proj.scale(swp, shp, 1.f);
    proj.translate(-scrollPos.x, -scrollPos.y, -FIXED_VIEW_DISTANCE);
    proj.scale(1.f, 1.f, ROOM_Z_SCALE);

    return glm::make_mat4(proj.constData());
}

} // namespace ProjectionUtils
