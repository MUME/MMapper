// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Characters.h"

#include "../configuration/configuration.h"
#include "../group/CGroupChar.h"
#include "../group/mmapper2group.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "MapCanvasData.h"
#include "Textures.h"
#include "mapcanvas.h"
#include "prespammedpath.h"

#include <array>
#include <cmath>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include <QtCore>

static constexpr float CHAR_ARROW_LINE_WIDTH = 2.f;
static constexpr float PATH_LINE_WIDTH = 0.1f;
static constexpr float PATH_POINT_SIZE = 8.f;

DistantObjectTransform DistantObjectTransform::construct(const glm::vec3 &pos,
                                                         const MapScreen &mapScreen,
                                                         const float marginPixels)
{
    assert(marginPixels > 0.f);
    const glm::vec3 viewCenter = mapScreen.getCenter();
    const auto delta = pos - viewCenter;
    const float radians = std::atan2(delta.y, delta.x);
    const auto hint = mapScreen.getProxyLocation(pos, marginPixels);
    const float degrees = glm::degrees(radians);
    return DistantObjectTransform{hint, degrees};
}

bool CharacterBatch::isVisible(const Coordinate &c, float margin) const
{
    return m_mapScreen.isRoomVisible(c, margin);
}

void CharacterBatch::drawCharacter(const Coordinate &c, const Color &color, bool fill)
{
    const Configuration::CanvasSettings &settings = getConfig().canvas;

    const glm::vec3 roomCenter = c.to_vec3() + glm::vec3{0.5f, 0.5f, 0.f};
    const int layerDifference = c.z - m_currentLayer;

    auto &gl = getOpenGL();
    gl.setColor(color);

    // REVISIT: The margin probably needs to be modified for high-dpi.
    const float marginPixels = MapScreen::DEFAULT_MARGIN_PIXELS;
    const bool visible = isVisible(c, marginPixels / 2.f);
    const bool isFar = m_scale <= settings.charBeaconScaleCutoff;
    const bool wantBeacons = settings.drawCharBeacons && isFar;
    if (!visible) {
        static const bool useScreenSpacePlayerArrow = []() -> bool {
            auto opt = utils::getEnvBool("MMAPPER_SCREEN_SPACE_ARROW");
            return opt ? opt.value() : true;
        }();
        const auto dot = DistantObjectTransform::construct(roomCenter, m_mapScreen, marginPixels);
        // Player is distant
        if (useScreenSpacePlayerArrow) {
            gl.addScreenSpaceArrow(dot.offset, dot.rotationDegrees, color, fill);
        } else {
            gl.glPushMatrix();
            gl.glTranslatef(dot.offset);
            // NOTE: 180 degrees of additional rotation flips the arrow to point right instead of left.
            gl.glRotateZ(dot.rotationDegrees + 180.f);
            // NOTE: arrow is centered, so it doesn't need additional translation.
            gl.drawArrow(fill, wantBeacons);
            gl.glPopMatrix();
        }
    }

    const bool differentLayer = layerDifference != 0;
    if (differentLayer) {
        const glm::vec3 centerOnCurrentLayer{static_cast<glm::vec2>(roomCenter),
                                             static_cast<float>(m_currentLayer)};
        // Draw any arrow on the current layer pointing in either up or down
        // (this may not make sense graphically in an angled 3D view).
        gl.glPushMatrix();
        gl.glTranslatef(centerOnCurrentLayer);
        // Arrow points up or down.
        // REVISIT: billboard this in 3D?
        gl.glRotateZ((layerDifference > 0) ? 90.f : 270.f);
        gl.drawArrow(fill, false);
        gl.glPopMatrix();
    }

    const bool beacon = visible && !differentLayer && wantBeacons;
    gl.drawBox(c, fill, beacon, isFar);
}

void CharacterBatch::CharFakeGL::drawPathSegment(const glm::vec3 &p1,
                                                 const glm::vec3 &p2,
                                                 const Color &color)
{
    // Calculate direction vector and perpendicular vector
    glm::vec3 direction = glm::normalize(p2 - p1);
    glm::vec3 perpendicular = glm::vec3(-direction.y, direction.x, 0.0f);

    glm::vec3 offset = perpendicular * (PATH_LINE_WIDTH * 0.5f);

    // Vertices for the quad
    glm::vec3 v1 = p1 + offset;
    glm::vec3 v2 = p1 - offset;
    glm::vec3 v3 = p2 - offset;
    glm::vec3 v4 = p2 + offset;

    // Add vertices to m_pathLineQuads as a quad
    m_pathLineQuads.emplace_back(color, v1);
    m_pathLineQuads.emplace_back(color, v2);
    m_pathLineQuads.emplace_back(color, v3);
    m_pathLineQuads.emplace_back(color, v4);
}

void CharacterBatch::drawPreSpammedPath(const Coordinate &c1,
                                        const std::vector<Coordinate> &path,
                                        const Color &color)
{
    if (path.empty()) {
        return;
    }

    const std::vector<glm::vec3> verts = [&c1, &path]() {
        std::vector<glm::vec3> translated;
        translated.reserve(static_cast<size_t>(path.size()) + 1);

        const auto add = [&translated](const Coordinate &c) -> void {
            static const glm::vec3 PATH_OFFSET{0.5f, 0.5f, 0.f};
            translated.push_back(c.to_vec3() + PATH_OFFSET);
        };

        add(c1);
        for (const Coordinate &c2 : path) {
            add(c2);
        }
        return translated;
    }();

    auto &gl = getOpenGL();

    // Generate vertices for the thick line
    for (size_t i = 0; i < verts.size() - 1; ++i) {
        const glm::vec3 p1 = verts[i];
        const glm::vec3 p2 = verts[i + 1];
        gl.drawPathSegment(p1, p2, color);
    }
    gl.drawPathPoint(color, verts.back());
}

void CharacterBatch::CharFakeGL::drawQuadCommon(const glm::vec2 &in_a,
                                                const glm::vec2 &in_b,
                                                const glm::vec2 &in_c,
                                                const glm::vec2 &in_d,
                                                const QuadOptsEnum options)
{
    const auto &m = m_stack.top().modelView;
    const auto transform = [&m](const glm::vec2 &vin) -> glm::vec3 {
        const auto vtmp = m * glm::vec4(vin, 0.f, 1.f);
        return glm::vec3{vtmp / vtmp.w};
    };

    const auto a = transform(in_a);
    const auto b = transform(in_b);
    const auto c = transform(in_c);
    const auto d = transform(in_d);

    if (::utils::isSet(options, QuadOptsEnum::FILL)) {
        const auto color = m_color.withAlpha(FILL_ALPHA);
        auto emitVert = [this, &color](const auto &x) -> void { m_charTris.emplace_back(color, x); };
        auto emitTri = [&emitVert](const auto &v0, const auto &v1, const auto &v2) -> void {
            emitVert(v0);
            emitVert(v1);
            emitVert(v2);
        };
        emitTri(a, b, c);
        emitTri(a, c, d);
    }

    if (::utils::isSet(options, QuadOptsEnum::BEACON)) {
        const auto color = m_color.withAlpha(BEACON_ALPHA);

        const glm::vec3 heightOffset{0.f, 0.f, 50.f};

        // H-----G
        // |\   /|
        // | D-C |
        // | | | |
        // | A-B |
        // |/   \|
        // E-----F

        const auto e = a + heightOffset;
        const auto f = b + heightOffset;
        const auto g = c + heightOffset;
        const auto h = d + heightOffset;

        auto emitVert = [this, &color](const auto &x) -> void {
            m_charBeaconQuads.emplace_back(color, x);
        };
        auto emitQuad =
            [&emitVert](const auto &v0, const auto &v1, const auto &v2, const auto &v3) -> void {
            emitVert(v0);
            emitVert(v1);
            emitVert(v2);
            emitVert(v3);
        };
        // draw the *inner* faces
        emitQuad(a, e, f, b);
        emitQuad(b, f, g, c);
        emitQuad(c, g, h, d);
        emitQuad(d, h, e, a);
    }

    if (::utils::isSet(options, QuadOptsEnum::OUTLINE)) {
        const auto color = m_color.withAlpha(LINE_ALPHA);
        auto emitVert = [this, &color](const auto &x) -> void {
            m_charLines.emplace_back(color, x);
        };
        auto emitLine = [&emitVert](const auto &v0, const auto &v1) -> void {
            emitVert(v0);
            emitVert(v1);
        };
        emitLine(a, b);
        emitLine(b, c);
        emitLine(c, d);
        emitLine(d, a);
    }
}

void CharacterBatch::CharFakeGL::drawBox(const Coordinate &coord,
                                         bool fill,
                                         bool beacon,
                                         const bool isFar)
{
    const bool dontFillRotatedQuads = true;
    const bool shrinkRotatedQuads = false; // REVISIT: make this a user option?

    // caution: multiple side effects in next statement.
    const int numAlreadyInRoom = m_coordCounts[coord]++;

    glPushMatrix();

    glTranslatef(coord.to_vec3());

    if (numAlreadyInRoom != 0) {
        // NOTE: use of 45/PI here is NOT a botched conversion to radians;
        // it's a value close to 15 degrees (~14.324) that is guaranteed
        // to never perfectly overlap a regular axis-aligned square
        // when multiplied by an integer.
        static constexpr const float MAGIC_ANGLE = 45.f / float(M_PI);
        const float degrees = static_cast<float>(numAlreadyInRoom) * MAGIC_ANGLE;
        const glm::vec3 quadCenter{0.5f, 0.5f, 0.f};
        glTranslatef(quadCenter);
        if ((shrinkRotatedQuads)) {
            // keeps the rotated squares bounded inside the outer square.
            glScalef(0.7f, 0.7f, 1.f);
        }
        glRotateZ(degrees);
        glTranslatef(-quadCenter);
        if (dontFillRotatedQuads) {
            fill = false; // avoid highlighting the room multiple times
        }
        beacon = false;
    }

    // d-c
    // |/|
    // a-b
    const glm::vec2 a{0, 0};
    const glm::vec2 b{1, 0};
    const glm::vec2 c{1, 1};
    const glm::vec2 d{0, 1};

    if (isFar) {
        const auto options = QuadOptsEnum::OUTLINE
                             | (fill ? QuadOptsEnum::FILL : QuadOptsEnum::NONE)
                             | (beacon ? QuadOptsEnum::BEACON : QuadOptsEnum::NONE);
        drawQuadCommon(a, b, c, d, options);
    } else {
        /* ignoring fill for now; that'll require a different icon */

        const auto &color = m_color;
        const auto &m = m_stack.top().modelView;
        const auto addTransformed = [this, &color, &m](const glm::vec2 &in_vert) -> void {
            const auto tmp = m * glm::vec4(in_vert, 0.f, 1.f);
            m_charRoomQuads.emplace_back(color, in_vert, glm::vec3{tmp / tmp.w});
        };
        addTransformed(a);
        addTransformed(b);
        addTransformed(c);
        addTransformed(d);

        if (beacon) {
            drawQuadCommon(a, b, c, d, QuadOptsEnum::BEACON);
        }
    }

    glPopMatrix();
}

void CharacterBatch::CharFakeGL::drawArrow(const bool fill, const bool beacon)
{
    // Generic topology:
    //    d
    //   /|
    //  a-c
    //   \|
    //    b

    const glm::vec2 a{-0.5f, 0.f};
    const glm::vec2 b{0.75f, -0.5f};
    const glm::vec2 c{0.25f, 0.f};
    const glm::vec2 d{0.75f, 0.5f};

    const auto options = QuadOptsEnum::OUTLINE | (fill ? QuadOptsEnum::FILL : QuadOptsEnum::NONE)
                         | (beacon ? QuadOptsEnum::BEACON : QuadOptsEnum::NONE);
    drawQuadCommon(a, b, c, d, options);
}

void CharacterBatch::CharFakeGL::reallyDrawCharacters(OpenGL &gl, const MapCanvasTextures &textures)
{
    const auto blended_noDepth
        = GLRenderState().withDepthFunction(std::nullopt).withBlend(BlendModeEnum::TRANSPARENCY);

    // Cull the front faces, because the quads point towards the center of the room,
    // and we don't want to draw over the entire terrain if we're inside the room.
    if (!m_charBeaconQuads.empty()) {
        gl.renderColoredQuads(m_charBeaconQuads, blended_noDepth.withCulling(CullingEnum::FRONT));
    }

    if (!m_charRoomQuads.empty()) {
        gl.renderColoredTexturedQuads(m_charRoomQuads,
                                      blended_noDepth.withTexture0(textures.char_room_sel->getId()));
    }

    if (!m_charTris.empty()) {
        gl.renderColoredTris(m_charTris, blended_noDepth);
    }

    if (!m_charLines.empty()) {
        gl.renderColoredLines(m_charLines,
                              blended_noDepth.withLineParams(LineParams{CHAR_ARROW_LINE_WIDTH}));
    }

    if (!m_screenSpaceArrows.empty()) {
        // FIXME: add an option to auto-scale to DPR.
        const float dpr = gl.getDevicePixelRatio();
        for (auto &v : m_screenSpaceArrows) {
            v.vert *= dpr;
        }
        gl.renderFont3d(textures.char_arrows, m_screenSpaceArrows);
        m_screenSpaceArrows.clear();
    }
}

void CharacterBatch::CharFakeGL::reallyDrawPaths(OpenGL &gl)
{
    const auto blended_noDepth
        = GLRenderState().withDepthFunction(std::nullopt).withBlend(BlendModeEnum::TRANSPARENCY);

    gl.renderPoints(m_pathPoints, blended_noDepth.withPointSize(PATH_POINT_SIZE));
    if (!m_pathLineQuads.empty()) {
        gl.renderColoredQuads(m_pathLineQuads, blended_noDepth);
    }
}

void CharacterBatch::CharFakeGL::addScreenSpaceArrow(const glm::vec3 &pos,
                                                     const float degrees,
                                                     const Color &color,
                                                     const bool fill)
{
    std::array<glm::vec2, 4> texCoords{
        glm::vec2{0, 0},
        glm::vec2{1, 0},
        glm::vec2{1, 1},
        glm::vec2{0, 1},
    };

    const float scale = MapScreen::DEFAULT_MARGIN_PIXELS;
    const float radians = glm::radians(degrees);
    const glm::vec3 z{0, 0, 1};
    const glm::mat4 rotation = glm::rotate(glm::mat4(1), radians, z);
    for (size_t i = 0; i < 4; ++i) {
        const glm::vec2 &tc = texCoords[i];
        const auto tmp = rotation * glm::vec4(tc * 2.f - 1.f, 0, 1);
        const glm::vec2 screenSpaceOffset = scale * glm::vec2(tmp) / tmp.w;
        // solid   |filled
        // --------+--------
        // outline | n/a
        const glm::vec2 tcOffset = tc * 0.5f
                                   + (fill ? glm::vec2(0.5f, 0.5f) : glm::vec2(0.f, 0.0f));
        m_screenSpaceArrows.emplace_back(pos, color, tcOffset, screenSpaceOffset);
    }
}

void MapCanvas::paintCharacters()
{
    if (m_data.isEmpty()) {
        return;
    }

    CharacterBatch characterBatch{m_mapScreen, m_currentLayer, getTotalScaleFactor()};

    // IIFE to abuse return to avoid duplicate else branches
    [this, &characterBatch]() {
        if (const std::optional<RoomId> opt_pos = m_data.getCurrentRoomId()) {
            const auto &id = opt_pos.value();
            if (const auto room = m_data.findRoomHandle(id)) {
                const auto &pos = room.getPosition();
                // draw the characters before the current position
                characterBatch.incrementCount(pos);
                drawGroupCharacters(characterBatch);
                characterBatch.resetCount(pos);

                // paint char current position
                const Color color{getConfig().groupManager.color};
                characterBatch.drawCharacter(pos, color);

                // paint prespam
                const auto prespam = m_data.getPath(id, m_prespammedPath.getQueue());
                characterBatch.drawPreSpammedPath(pos, prespam, color);
                return;
            } else {
                // this can happen if the "current room" is deleted
                // and we failed to clear it elsewhere.
                m_data.clearSelectedRoom();
            }
        }
        drawGroupCharacters(characterBatch);
    }();

    characterBatch.reallyDraw(getOpenGL(), m_textures);
}

void MapCanvas::drawGroupCharacters(CharacterBatch &batch)
{
    if (m_data.isEmpty()) {
        return;
    }

    RoomIdSet drawnRoomIds;
    const Map &map = m_data.getCurrentMap();
    for (const auto &pCharacter : m_groupManager.selectAll()) {
        // Omit player so that they know group members are below them
        if (pCharacter->isYou())
            continue;

        const CGroupChar &character = deref(pCharacter);

        const auto &r = [&character, &map]() -> RoomHandle {
            const ServerRoomId srvId = character.getServerId();
            if (srvId != INVALID_SERVER_ROOMID) {
                if (const auto &room = map.findRoomHandle(srvId)) {
                    return room;
                }
            }
            return RoomHandle{};
        }();

        // Do not draw the character if they're in an "Unknown" room
        if (!r) {
            continue;
        }

        const RoomId id = r.getId();
        const auto &pos = r.getPosition();
        const auto color = Color{character.getColor()};
        const bool fill = !drawnRoomIds.contains(id);

        batch.drawCharacter(pos, color, fill);
        drawnRoomIds.insert(id);
    }
}
