#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include <glm/glm.hpp>

/**
 * @brief Configuration for viewport projection matrix calculation.
 * Encapsulates settings from the higher-level UI to keep projection utilities independent.
 */
struct ViewportConfig
{
    bool use3D = false;
    bool autoTilt = true;
    float fovDegrees = 45.f;
    float verticalAngleDegrees = 45.f;
    float horizontalAngleDegrees = 0.f;
    float layerHeight = 7.f;
};

namespace ProjectionUtils {

/**
 * @brief Calculate the pitch (vertical angle) in degrees, accounting for auto-tilt if enabled.
 */
float calculatePitchDegrees(const ViewportConfig &config, float zoomScale);

/**
 * @brief Modern 3D projection matrix calculation.
 */
glm::mat4 calculateViewProj(const ViewportConfig &config,
                            const glm::vec2 &scrollPos,
                            const glm::ivec2 &size,
                            float zoomScale,
                            int currentLayer);

/**
 * @brief Legacy 2D-style projection matrix calculation.
 */
glm::mat4 calculateViewProjOld(const glm::vec2 &scrollPos,
                               const glm::ivec2 &size,
                               float zoomScale,
                               int currentLayer);

} // namespace ProjectionUtils
