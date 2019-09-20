// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MapCanvasData.h"

#include <optional>
#include <QPointF>

const MMapper::Array<RoomTintEnum, NUM_ROOM_TINTS> &getAllRoomTints()
{
    static const MMapper::Array<RoomTintEnum, NUM_ROOM_TINTS>
        all_room_tints{RoomTintEnum::DARK, RoomTintEnum::NO_SUNDEATH};
    return all_room_tints;
}

MapCanvasInputState::MapCanvasInputState(PrespammedPath *const prespammedPath)
    : m_prespammedPath{prespammedPath}
{}

MapCanvasInputState::~MapCanvasInputState() = default;

// world space to screen space (logical pixels)
std::optional<glm::vec3> MapCanvasViewport::project(const glm::vec3 &v) const
{
    const auto tmp = m_viewProj * glm::vec4(v, 1.f);

    // This can happen if you set the layer height to the view distance
    // and then try to project a point on layer = 1, when the vertical
    // angle is 1, so the plane would pass through the camera.
    if (std::abs(tmp.w) < 1e-6f) {
        return std::nullopt;
    }
    const auto ndc = glm::vec3{tmp} / tmp.w; /* [-1, 1]^3 if clamped */

    const float epsilon = 1e-5f;
    if (glm::any(glm::greaterThan(glm::abs(ndc), glm::vec3{1.f + epsilon}))) {
        // result is not visible on screen.
        return std::nullopt;
    }

    const auto screen = glm::clamp(ndc * 0.5f + 0.5f, 0.f, 1.f); /* [0, 1]^3 if clamped */

    const Viewport viewport = getViewport();
    const auto mouse = glm::vec2{screen} * glm::vec2{viewport.size} + glm::vec2{viewport.offset};
    const glm::vec3 mouse_depth{mouse, screen.z};
    return mouse_depth;
}

// input: 2d mouse coordinates clamped in viewport_offset + [0..viewport_size]
// and a depth value in the range 0..1.
//
// output: world coordinates.
glm::vec3 MapCanvasViewport::unproject_raw(const glm::vec3 &mouse_depth) const
{
    const float depth = mouse_depth.z;
    assert(::isClamped(depth, 0.f, 1.f));

    const Viewport viewport = getViewport();
    const glm::vec2 mouse{mouse_depth};
    const auto screen2d = (mouse - glm::vec2{viewport.offset}) / glm::vec2{viewport.size};
    const glm::vec3 screen{screen2d, depth};
    const auto ndc = screen * 2.f - 1.f;

    const auto tmp = glm::inverse(m_viewProj) * glm::vec4(ndc, 1.f);
    // clamp to avoid division by zero
    constexpr float limit = 1e-6f;
    const auto w = (std::abs(tmp.w) < limit) ? std::copysign(limit, tmp.w) : tmp.w;
    const auto world = glm::vec3{tmp} / w;
    return world;
}

// Returns a value on the current layer;
// note: the returned coordinate may not be visible,
// because it could be
glm::vec3 MapCanvasViewport::unproject_clamped(const glm::vec2 &mouse) const
{
    const auto flayer = static_cast<float>(m_currentLayer);
    const auto &x = mouse.x;
    const auto &y = mouse.y;
    const auto a = unproject_raw(glm::vec3{x, y, 0.f}); // near
    const auto b = unproject_raw(glm::vec3{x, y, 1.f}); // far
    const float t = (flayer - a.z) / (b.z - a.z);
    const auto result = glm::mix(a, b, std::clamp(t, 0.f, 1.f));
    return glm::vec3{glm::vec2{result}, flayer};
}

glm::vec2 MapCanvasViewport::getMouseCoords(const QInputEvent *const event) const
{
    if (const auto *const mouse = dynamic_cast<const QMouseEvent *>(event)) {
        const auto x = static_cast<float>(mouse->pos().x());
        const auto y = static_cast<float>(height() - mouse->pos().y());
        return glm::vec2{x, y};
    } else if (const auto *const wheel = dynamic_cast<const QWheelEvent *>(event)) {
        const auto x = static_cast<float>(wheel->pos().x());
        const auto y = static_cast<float>(height() - wheel->pos().y());
        return glm::vec2{x, y};
    } else {
        throw std::invalid_argument("event");
    }
}

// input: screen coordinates;
// output is the world space intersection with the current layer
std::optional<glm::vec3> MapCanvasViewport::unproject(const QInputEvent *const event) const
{
    const auto xy = getMouseCoords(event);
    // We don't actually know the depth we're trying to unproject;
    // technically we're solving for a ray, so we should unproject
    // two different depths and find where the ray intersects the
    // current layer.

    const auto a = unproject_raw(glm::vec3{xy, 0.f}); // near
    const auto b = unproject_raw(glm::vec3{xy, 1.f}); // far
    const auto unclamped = (static_cast<float>(m_currentLayer) - a.z) / (b.z - a.z);

    const float epsilon = 1e-5f; // allow a small amount of overshoot
    if (!::isClamped(unclamped, 0.f - epsilon, 1.f + epsilon)) {
        return std::nullopt;
    }

    // REVISIT: set the z value exactly to m_currentLayer?
    // (Note: caller ignores Z and uses integer value for current layer)
    return glm::mix(a, b, glm::clamp(unclamped, 0.f, 1.f));
}

std::optional<MouseSel> MapCanvasViewport::getUnprojectedMouseSel(const QInputEvent *const event) const
{
    const auto opt_v = unproject(event);
    if (!opt_v.has_value())
        return std::nullopt;
    const glm::vec3 &v = opt_v.value();
    return MouseSel{Coordinate2f{v.x, v.y}, m_currentLayer};
}

MapScreen::MapScreen(const MapCanvasViewport &viewport)
    : m_viewport{viewport}
{}

MapScreen::~MapScreen() = default;

glm::vec3 MapScreen::getCenter() const
{
    const Viewport vp = m_viewport.getViewport();
    return m_viewport.unproject_clamped(glm::vec2{vp.offset} + glm::vec2{vp.size} * 0.5f);
}

bool MapScreen::isRoomVisible(const Coordinate &c, const float marginPixels) const
{
    const auto pos = c.to_vec3();
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 offset{static_cast<float>(i & 1), static_cast<float>((i >> 1) & 1), 0.f};
        const auto corner = pos + offset;
        switch (testVisibility(corner, marginPixels)) {
        case VisiblityResultEnum::INSIDE_MARGIN:
        case VisiblityResultEnum::ON_MARGIN:
            break;

        case VisiblityResultEnum::OUTSIDE_MARGIN:
        case VisiblityResultEnum::OFF_SCREEN:
            return false;
        }
    }

    return true;
}

// Purposely ignores the possibility of glClipPlane() and glDepthRange().
MapScreen::VisiblityResultEnum MapScreen::testVisibility(const glm::vec3 &input_pos,
                                                         const float marginPixels) const
{
    assert(marginPixels >= 1.f);

    const auto &viewport = m_viewport;
    const auto opt_mouse = viewport.project(input_pos);
    if (!opt_mouse.has_value()) {
        return VisiblityResultEnum::OFF_SCREEN;
    }

    // NOTE: From now on, we'll ignore depth because we know it's "on screen."
    const auto vp = viewport.getViewport();
    const glm::vec2 offset{vp.offset};
    const glm::vec2 size{vp.size};
    const glm::vec2 half_size = size * 0.5f;
    const auto &mouse_depth = opt_mouse.value();
    const glm::vec2 mouse = glm::vec2(mouse_depth) - offset;

    // for height 480, height/2 is 240, and then:
    // e.g.
    //   240 - abs(5 - 240) = 5 pixels
    //   240 - abs(475 - 240) = 5 pixels
    const glm::vec2 d = half_size - glm::abs(mouse - half_size);

    // We want the minimum value (closest to the edge).
    const auto dist = std::min(d.x, d.y);

    // e.g. if margin is 20.0, then floorMargin is 20, and ceilMargin is 21.0
    const float floorMargin = std::floor(marginPixels);
    const float ceilMargin = floorMargin + 1.f;

    // larger values are "more inside".
    // e.g.
    //   distance 5 vs margin 20 is "outside",
    //   distance 25 vs margin 20 is "inside".
    if (dist < floorMargin)
        return VisiblityResultEnum::OUTSIDE_MARGIN;
    else if (dist > ceilMargin)
        return VisiblityResultEnum::INSIDE_MARGIN;
    else
        return VisiblityResultEnum::ON_MARGIN;
}

glm::vec3 MapScreen::getProxyLocation(const glm::vec3 &input_pos, const float marginPixels) const
{
    const auto center = this->getCenter();

    const VisiblityResultEnum abs_result = testVisibility(input_pos, marginPixels);
    switch (abs_result) {
    case VisiblityResultEnum::INSIDE_MARGIN:
    case VisiblityResultEnum::ON_MARGIN:
        return input_pos;
    case VisiblityResultEnum::OUTSIDE_MARGIN:
    case VisiblityResultEnum::OFF_SCREEN:
        break;
    }

    float proxyFraction = 0.5f;
    float stepFraction = 0.25f;
    constexpr int maxSteps = 23;
    glm::vec3 bestInside = center;
    float bestInsideFraction = 0.f;
    // clang-tidy is confused here. The loop induction variable is an integer.
    for (int i = 0; i < maxSteps; ++i, stepFraction *= 0.5f) {
        const auto tmp_pos = glm::mix(center, input_pos, proxyFraction);
        const VisiblityResultEnum tmp_result = testVisibility(tmp_pos, marginPixels);
        switch (tmp_result) {
        case VisiblityResultEnum::INSIDE_MARGIN:
            // Once we've hit "inside", math tells us it should never end up
            // hitting inside with a lower value, but let's guard just to be safe.
            assert(proxyFraction > bestInsideFraction);
            if (proxyFraction > bestInsideFraction) {
                bestInside = tmp_pos;
                bestInsideFraction = proxyFraction;
            }
            proxyFraction += stepFraction;
            break;
        case VisiblityResultEnum::ON_MARGIN:
            return tmp_pos;
        case VisiblityResultEnum::OUTSIDE_MARGIN:
        case VisiblityResultEnum::OFF_SCREEN:
            proxyFraction -= stepFraction;
            break;
        }
    }

    // This really should never happen, because it means we visited 23 bits
    // of mantissa without finding one on the margin.
    // Here's our best guess.
    return bestInside;
}
