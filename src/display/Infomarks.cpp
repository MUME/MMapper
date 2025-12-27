// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Infomarks.h"

#include "../configuration/configuration.h"
#include "../global/Charset.h"
#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../mapdata/mapdata.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/LineRendering.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "InfomarkSelection.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "connectionselection.h"
#include "mapcanvas.h"

#include <cassert>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/norm.hpp>

#include <QMessageLogContext>

static constexpr const float INFOMARK_ARROW_LINE_WIDTH = 0.045f;
static constexpr float INFOMARK_GUIDE_LINE_WIDTH = 3.f;
static constexpr float INFOMARK_POINT_SIZE = 6.f;

#define LOOKUP_COLOR_INFOMARK(_X) (XNamedColor{NamedColorEnum::_X}.getColor())

// NOTE: This currently requires rebuilding the infomark meshes if a color changes.
NODISCARD static Color getInfomarkColor(const InfomarkTypeEnum infoMarkType,
                                        const InfomarkClassEnum infoMarkClass)
{
    const Color defaultColor = (infoMarkType == InfomarkTypeEnum::TEXT) ? Colors::black
                                                                        : Colors::white;
    switch (infoMarkClass) {
    case InfomarkClassEnum::HERB:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_HERB);
    case InfomarkClassEnum::RIVER:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_RIVER);
    case InfomarkClassEnum::MOB:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_MOB);
    case InfomarkClassEnum::COMMENT:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_COMMENT);
    case InfomarkClassEnum::ROAD:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_ROAD);
    case InfomarkClassEnum::OBJECT:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_OBJECT);

    case InfomarkClassEnum::GENERIC:
    case InfomarkClassEnum::PLACE:
    case InfomarkClassEnum::ACTION:
    case InfomarkClassEnum::LOCALITY:
        return defaultColor;
    }

    assert(false);
    return defaultColor;
}

NODISCARD static FontFormatFlags getFontFormatFlags(const InfomarkClassEnum infoMarkClass)
{
    switch (infoMarkClass) {
    case InfomarkClassEnum::GENERIC:
    case InfomarkClassEnum::HERB:
    case InfomarkClassEnum::RIVER:
    case InfomarkClassEnum::PLACE:
    case InfomarkClassEnum::MOB:
    case InfomarkClassEnum::COMMENT:
    case InfomarkClassEnum::ROAD:
    case InfomarkClassEnum::OBJECT:
        return {};

    case InfomarkClassEnum::ACTION:
        return FontFormatFlags{FontFormatFlagEnum::ITALICS};

    case InfomarkClassEnum::LOCALITY:
        return FontFormatFlags{FontFormatFlagEnum::UNDERLINE};
    }

    assert(false);
    return {};
}

BatchedInfomarksMeshes MapCanvas::getInfomarksMeshes()
{
    BatchedInfomarksMeshes result;
    {
        const auto &map = m_data.getCurrentMap();
        const auto &db = map.getInfomarkDb();
        db.getIdSet().for_each([&](const InfomarkId id) {
            InfomarkHandle mark{db, id};
            const int layer = mark.getPosition1().z;
            const auto it = result.find(layer);
            if (it == result.end()) {
                std::ignore = result[layer]; // side effect: this creates the entry
            }
        });
    }

    if (result.size() >= 30) {
        qWarning() << "Infomarks span" << result.size()
                   << "layers. Consider using a different algorithm if this function is too slow.";
    }

    // WARNING: This is O(layers) * O(markers), which is okay as long
    // as the number of layers with infomarks is small.
    //
    // If the performance gets too bad, count # in each layer,
    // allocate vectors, fill the vectors, and then only visit
    // each one once per layer.
    const auto &map = m_data.getCurrentMap();
    const auto &db = map.getInfomarkDb();
    for (auto &it : result) {
        const int layer = it.first;
        InfomarksBatch batch{getOpenGL(), getGLFont()};
        db.getIdSet().for_each(
            [&](const InfomarkId id) { drawInfomark(batch, InfomarkHandle{db, id}, layer); });
        it.second = batch.getMeshes();
    }

    return result;
}

void InfomarksBatch::drawPoint(const glm::vec3 &a)
{
    m_points.emplace_back(m_color, a + m_offset);
}

void InfomarksBatch::drawLine(const glm::vec3 &a, const glm::vec3 &b)
{
    const glm::vec3 start_v = a + m_offset;
    const glm::vec3 end_v = b + m_offset;

    mmgl::generateLineQuadsSafe(m_quads, start_v, end_v, INFOMARK_ARROW_LINE_WIDTH, m_color);
}

void InfomarksBatch::drawTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
{
    m_tris.emplace_back(m_color, a + m_offset);
    m_tris.emplace_back(m_color, b + m_offset);
    m_tris.emplace_back(m_color, c + m_offset);
}

void InfomarksBatch::renderText(const glm::vec3 &pos,
                                const std::string &text,
                                const Color color,
                                const std::optional<Color> bgcolor,
                                const FontFormatFlags fontFormatFlag,
                                const int rotationAngle)
{
    assert(!m_text.locked);
    m_text.text.emplace_back(pos, text, color, bgcolor, fontFormatFlag, rotationAngle);
}

InfomarksMeshes InfomarksBatch::getMeshes()
{
    InfomarksMeshes result;

    auto &gl = m_realGL;
    result.points = gl.createPointBatch(m_points);
    result.tris = gl.createColoredTriBatch(m_tris);
    result.quads = gl.createColoredQuadBatch(m_quads);

    {
        assert(!m_text.locked);
        m_text.verts = m_font.getFontMeshIntermediate(m_text.text);
        m_text.locked = true;
    }
    result.textMesh = m_font.getFontMesh(m_text.verts);

    result.isValid = true;

    return result;
}

void InfomarksBatch::renderImmediate(const GLRenderState &state)
{
    auto &gl = m_realGL;

    if (!m_tris.empty()) {
        gl.renderColoredTris(m_tris, state);
    }
    if (!m_quads.empty()) {
        gl.renderColoredQuads(m_quads, state);
    }
    if (!m_text.text.empty()) {
        m_font.render3dTextImmediate(m_text.text);
    }
    if (!m_points.empty()) {
        gl.renderPoints(m_points, state.withPointSize(INFOMARK_POINT_SIZE));
    }
}

void InfomarksMeshes::render()
{
    if (!isValid) {
        return;
    }

    const auto common_state
        = GLRenderState().withDepthFunction(std::nullopt).withBlend(BlendModeEnum::TRANSPARENCY);

    points.render(common_state.withPointSize(INFOMARK_POINT_SIZE));
    tris.render(common_state);
    quads.render(common_state);
    textMesh.render(common_state);
}

void MapCanvas::drawInfomark(InfomarksBatch &batch,
                             const InfomarkHandle &marker,
                             const int currentLayer,
                             const glm::vec2 &offset,
                             const std::optional<Color> &overrideColor)
{
    if (!marker.exists()) {
        assert(false);
        return;
    }

    const Coordinate &pos1 = marker.getPosition1();
    const int layer = pos1.z;
    if (layer != currentLayer) {
        // REVISIT: consider storing infomarks by level
        // so we don't have to test here.
        return;
    }

    const Coordinate &pos2 = marker.getPosition2();
    const float x1 = static_cast<float>(pos1.x) / INFOMARK_SCALE + offset.x;
    const float y1 = static_cast<float>(pos1.y) / INFOMARK_SCALE + offset.y;
    const float x2 = static_cast<float>(pos2.x) / INFOMARK_SCALE + offset.x;
    const float y2 = static_cast<float>(pos2.y) / INFOMARK_SCALE + offset.y;
    const float dx = x2 - x1;
    const float dy = y2 - y1;

    const auto infoMarkType = marker.getType();
    const auto infoMarkClass = marker.getClass();

    // Color depends of the class of the Infomark
    const Color infoMarkColor = getInfomarkColor(infoMarkType, infoMarkClass).withAlpha(0.55f);
    const auto fontFormatFlag = getFontFormatFlags(infoMarkClass);

    const glm::vec3 pos{x1, y1, static_cast<float>(layer)};
    batch.setOffset(pos);

    const Color bgColor = (overrideColor) ? overrideColor.value() : infoMarkColor;
    batch.setColor(bgColor);

    switch (infoMarkType) {
    case InfomarkTypeEnum::TEXT: {
        const auto utf8 = marker.getText().getStdStringViewUtf8();
        const auto latin1_to_render = charset::conversion::utf8ToLatin1(utf8); // GL font is latin1
        batch.renderText(pos,
                         latin1_to_render,
                         textColor(bgColor),
                         bgColor,
                         fontFormatFlag,
                         marker.getRotationAngle());
        break;
    }

    case InfomarkTypeEnum::LINE:
        batch.drawLine(glm::vec3{0.f}, glm::vec3{dx, dy, 0.f});
        break;

    case InfomarkTypeEnum::ARROW:
        // Draw the main shaft line quad, extending it to the arrowhead's base
        batch.drawLine(glm::vec3{0.f}, glm::vec3{dx - 0.2f, dy, 0.f});

        // Draw the arrowhead triangle
        batch.drawTriangle(glm::vec3{dx - 0.2f, dy + 0.07f, 0.f},
                           glm::vec3{dx - 0.2f, dy - 0.07f, 0.f},
                           glm::vec3{dx, dy, 0.f});
        break;
    }
}

void MapCanvas::paintNewInfomarkSelection()
{
    if (!hasSel1() || !hasSel2()) {
        return;
    }

    const auto pos1 = getSel1().pos.to_vec2();
    const auto pos2 = getSel2().pos.to_vec2();

    // Mouse selected area
    auto &gl = getOpenGL();
    const auto layer = static_cast<float>(m_currentLayer);

    // Draw yellow guide when creating an infomark line/arrow
    if (m_canvasMouseMode == CanvasMouseModeEnum::CREATE_INFOMARKS && m_selectedArea) {
        const auto infomarksLineStyle = GLRenderState()
                                            .withColor(Color{Qt::yellow})
                                            .withLineParams(LineParams{INFOMARK_GUIDE_LINE_WIDTH});
        const std::vector<glm::vec3> verts{glm::vec3{pos1, layer}, glm::vec3{pos2, layer}};
        gl.renderPlainLines(verts, infomarksLineStyle);
    }
}

void MapCanvas::paintSelectedInfomarks()
{
    if (m_infoMarkSelection == nullptr
        && m_canvasMouseMode != CanvasMouseModeEnum::SELECT_INFOMARKS) {
        return;
    }

    InfomarksBatch batch{getOpenGL(), getGLFont()};
    {
        // draw selections
        if (m_infoMarkSelection != nullptr) {
            const InfomarkSelection &sel = deref(m_infoMarkSelection);
            sel.for_each([this, &batch](const InfomarkHandle &marker) {
                drawInfomark(batch, marker, m_currentLayer, {}, Colors::red);
            });
            if (hasInfomarkSelectionMove()) {
                const glm::vec2 offset = m_infoMarkSelectionMove->pos.to_vec2();
                sel.for_each([this, &batch, &offset](const InfomarkHandle &marker) {
                    drawInfomark(batch, marker, m_currentLayer, offset, Colors::yellow);
                });
            }
        }

        // draw selection points
        if (m_canvasMouseMode == CanvasMouseModeEnum::SELECT_INFOMARKS) {
            const auto drawPoint = [&batch](const Coordinate &c, const Color color) {
                batch.setColor(color);
                batch.setOffset(glm::vec3{0});
                const glm::vec3 point{static_cast<glm::vec2>(c.to_vec3())
                                          / static_cast<float>(INFOMARK_SCALE),
                                      c.z};
                batch.drawPoint(point);
            };

            const auto drawSelectionPoints = [this, &drawPoint](const InfomarkHandle &marker) {
                const auto &pos1 = marker.getPosition1();
                if (pos1.z != m_currentLayer) {
                    return;
                }

                const auto color = (m_infoMarkSelection != nullptr
                                    && m_infoMarkSelection->contains(marker.getId()))
                                       ? Colors::yellow
                                       : Colors::cyan;

                drawPoint(pos1, color);
                if (marker.getType() == InfomarkTypeEnum::TEXT) {
                    return;
                }

                const Coordinate &pos2 = marker.getPosition2();
                assert(pos2.z == m_currentLayer);
                drawPoint(pos2, color);
            };

            const auto &map = m_data.getCurrentMap();
            const InfomarkDb &db = map.getInfomarkDb();
            db.getIdSet().for_each([&](const InfomarkId id) {
                InfomarkHandle marker{db, id};
                drawSelectionPoints(marker);
            });
        }
    }
    batch.renderImmediate(GLRenderState());
}

void MapCanvas::paintBatchedInfomarks()
{
    const auto wantInfomarks = (getTotalScaleFactor() >= getConfig().canvas.infomarkScaleCutoff);
    if (!wantInfomarks || !m_batches.infomarksMeshes.has_value()) {
        return;
    }

    BatchedInfomarksMeshes &map = m_batches.infomarksMeshes.value();
    const auto it = map.find(static_cast<int>(m_currentLayer));
    if (it == map.end()) {
        return;
    }

    InfomarksMeshes &infomarksMeshes = it->second;
    infomarksMeshes.render();
}

void MapCanvas::updateInfomarkBatches()
{
    std::optional<BatchedInfomarksMeshes> &opt_infomarks = m_batches.infomarksMeshes;
    if (opt_infomarks.has_value() && !m_data.getNeedsMarkUpdate()) {
        return;
    }

    if (m_data.getNeedsMarkUpdate()) {
        m_data.clearNeedsMarkUpdate();
        assert(!m_data.getNeedsMarkUpdate());
        MMLOG() << "[updateInfomarkBatches] cleared 'needsUpdate' flag";
    }

    opt_infomarks.emplace(getInfomarksMeshes());
}
