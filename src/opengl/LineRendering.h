// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#pragma once

#include "../opengl/OpenGLTypes.h"

#include <vector>

#include <glm/glm.hpp>

namespace mmgl {

// Tolerance for projecting world coordinates to screen space.
// Small but non-zero w values can cause numerical instability if used as divisors.
// A threshold of 1e-6f is a balance between precision and avoiding noise amplification.
static constexpr const float W_PROJECTION_EPSILON = 1e-6f;

// Geometric epsilon for degeneracy checks (e.g., near-zero vectors, collinearity).
// This is used for comparisons where small floating-point variations should be treated as equivalent to zero.
static constexpr const float GEOMETRIC_EPSILON = 1e-5f;

// Projection epsilon for clamping logic in screen space.
// This handles numerical instability during world-to-screen projections.
static constexpr const float PROJECTION_EPSILON = 1e-5f;

// Squared threshold for zero-length segment checks to avoid sqrt operations.
static constexpr const float ZERO_LENGTH_THRESHOLD_SQ = GEOMETRIC_EPSILON * GEOMETRIC_EPSILON;

// NOTE: perpendicular_normal is assumed to be unit length.
void generateLineQuad(std::vector<ColorVert> &verts,
                      const glm::vec3 &p1,
                      const glm::vec3 &p2,
                      float width,
                      const Color &color,
                      const glm::vec3 &perpendicular_normal);

void drawZeroLengthSquare(std::vector<ColorVert> &verts,
                          const glm::vec3 &center,
                          float width,
                          const Color &color);

// Returns a normalized vector perpendicular to the input direction, primarily in the XY plane.
// Handles near-zero direction vectors by returning a default perpendicular (1,0,0).
NODISCARD glm::vec3 getPerpendicularNormal(const glm::vec3 &direction);

// Returns a normalized vector orthogonal to both the segment direction and the first perpendicular normal.
// This is suitable for generating a second quad to form a "cross" shape.
NODISCARD glm::vec3 getOrthogonalNormal(const glm::vec3 &direction, const glm::vec3 &perp_normal_1);

// Generates a line quad, handling zero-length segments by drawing a square instead.
// Uses getPerpendicularNormal for the quad generation.
void generateLineQuadsSafe(std::vector<ColorVert> &verts,
                           const glm::vec3 &p1,
                           const glm::vec3 &p2,
                           float width,
                           const Color &color);

// Checks if the squared length of a vector is below the degeneration threshold.
NODISCARD bool isDegenerate(const glm::vec3 &vec);

// Checks if the squared length of a segment vector is below the zero-length threshold.
NODISCARD bool isNearZero(const glm::vec3 &segment);

} // namespace mmgl
