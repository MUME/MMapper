// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "LineRendering.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

namespace mmgl {

void generateLineQuad(std::vector<ColorVert> &verts,
                      const glm::vec3 &p1,
                      const glm::vec3 &p2,
                      const float width,
                      const Color color,
                      const glm::vec3 &perpendicular_normal)
{
    // Assert that perpendicular_normal is unit length
    assert(
        glm::epsilonEqual(glm::length2(perpendicular_normal), 1.0f, mmgl::GEOMETRIC_EPSILON * 10.f));

    const float half_width = width / 2.0f;
    glm::vec3 offset1 = perpendicular_normal * half_width;

    // Use p1 and p2 directly for quad vertices, applying the perpendicular offset.
    glm::vec3 v1 = p1 + offset1;
    glm::vec3 v2 = p1 - offset1;
    glm::vec3 v3 = p2 - offset1;
    glm::vec3 v4 = p2 + offset1;

    verts.emplace_back(color, v1);
    verts.emplace_back(color, v2);
    verts.emplace_back(color, v3);
    verts.emplace_back(color, v4);
}

bool isDegenerate(const glm::vec3 &vec)
{
    return glm::length2(vec) < mmgl::GEOMETRIC_EPSILON * 10.f;
}

bool isNearZero(const glm::vec3 &segment)
{
    return glm::length2(segment) < mmgl::ZERO_LENGTH_THRESHOLD_SQ;
}

glm::vec3 getPerpendicularNormal(const glm::vec3 &direction)
{
    glm::vec3 perp_normal_candidate = glm::vec3(-direction.y, direction.x, 0.0f);
    if (isDegenerate(perp_normal_candidate)) {
        return glm::vec3(1.0f, 0.0f, 0.0f);
    }
    return glm::normalize(perp_normal_candidate);
}

glm::vec3 getOrthogonalNormal(const glm::vec3 &direction, const glm::vec3 &perp_normal_1)
{
    glm::vec3 perp_normal_2_candidate = glm::cross(direction, perp_normal_1);
    if (isDegenerate(perp_normal_2_candidate)) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return glm::normalize(perp_normal_2_candidate);
}

void generateLineQuadsSafe(std::vector<ColorVert> &verts,
                           const glm::vec3 &p1,
                           const glm::vec3 &p2,
                           const float width,
                           const Color color)
{
    const glm::vec3 segment = p2 - p1;
    if (isNearZero(segment)) {
        drawZeroLengthSquare(verts, p1, width, color);
        return;
    }

    const glm::vec3 normalized_dir = glm::normalize(segment);
    const glm::vec3 perp_normal = getPerpendicularNormal(normalized_dir);
    generateLineQuad(verts, p1, p2, width, color, perp_normal);
}

void drawZeroLengthSquare(std::vector<ColorVert> &verts,
                          const glm::vec3 &center,
                          float width,
                          const Color color)
{
    float half_size = width / 2.0f;
    glm::vec3 v1 = center + glm::vec3(-half_size, -half_size, 0.0f);
    glm::vec3 v2 = center + glm::vec3(half_size, -half_size, 0.0f);
    glm::vec3 v3 = center + glm::vec3(half_size, half_size, 0.0f);
    glm::vec3 v4 = center + glm::vec3(-half_size, half_size, 0.0f);

    verts.emplace_back(color, v1);
    verts.emplace_back(color, v2);
    verts.emplace_back(color, v3);
    verts.emplace_back(color, v4);
}

} // namespace mmgl
