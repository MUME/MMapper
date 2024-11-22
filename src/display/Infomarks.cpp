// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Infomarks.h"

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../global/AnsiTextUtils.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "InfoMarkSelection.h"
#include "MapCanvasData.h"
#include "MapCanvasRoomDrawer.h"
#include "connectionselection.h"
#include "mapcanvas.h"

#include <cassert>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include <QMessageLogContext>
#include <QtCore>

static constexpr const float INFOMARK_ARROW_LINE_WIDTH = 2.f;
static constexpr float INFOMARK_GUIDE_LINE_WIDTH = 3.f;
static constexpr float INFOMARK_POINT_SIZE = 6.f;

#define LOOKUP_COLOR_INFOMARK(X) (getConfig().colorSettings.X.getColor())

// NOTE: This currently requires rebuilding the infomark meshes if a color changes.
NODISCARD static Color getInfoMarkColor(const InfoMarkTypeEnum infoMarkType,
                                        const InfoMarkClassEnum infoMarkClass)
{
    const Color defaultColor = (infoMarkType == InfoMarkTypeEnum::TEXT) ? Colors::black
                                                                        : Colors::white;
    switch (infoMarkClass) {
    case InfoMarkClassEnum::HERB:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_HERB);
    case InfoMarkClassEnum::RIVER:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_RIVER);
    case InfoMarkClassEnum::MOB:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_MOB);
    case InfoMarkClassEnum::COMMENT:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_COMMENT);
    case InfoMarkClassEnum::ROAD:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_ROAD);
    case InfoMarkClassEnum::OBJECT:
        return LOOKUP_COLOR_INFOMARK(INFOMARK_OBJECT);

    case InfoMarkClassEnum::GENERIC:
    case InfoMarkClassEnum::PLACE:
    case InfoMarkClassEnum::ACTION:
    case InfoMarkClassEnum::LOCALITY:
        return defaultColor;
    }

    assert(false);
    return defaultColor;
}

NODISCARD static FontFormatFlags getFontFormatFlags(const InfoMarkClassEnum infoMarkClass)
{
    switch (infoMarkClass) {
    case InfoMarkClassEnum::GENERIC:
    case InfoMarkClassEnum::HERB:
    case InfoMarkClassEnum::RIVER:
    case InfoMarkClassEnum::PLACE:
    case InfoMarkClassEnum::MOB:
    case InfoMarkClassEnum::COMMENT:
    case InfoMarkClassEnum::ROAD:
    case InfoMarkClassEnum::OBJECT:
        return {};

    case InfoMarkClassEnum::ACTION:
        return FontFormatFlags{FontFormatFlagEnum::ITALICS};

    case InfoMarkClassEnum::LOCALITY:
        return FontFormatFlags{FontFormatFlagEnum::UNDERLINE};
    }

    assert(false);
    return {};
}

BatchedInfomarksMeshes MapCanvas::getInfoMarksMeshes()
{
    // REVISIT: Infomarks uses QLinkedList. Linked list iteration can be
    // an order of magnitude slower than vector iteration.
    //
    // Consider converting the infomarks data structure to std::vector.

    BatchedInfomarksMeshes result;
    {
        for (const auto &mark : m_data.getMarkersList()) {
            const int layer = mark->getPosition1().z;
            const auto it = result.find(layer);
            if (it == result.end())
                result[layer]; // side effect: this creates the entry
        }
    }

    if (result.size() >= 30)
        qWarning() << "Infomarks span" << result.size()
                   << "layers. Consider using a different algorithm if this function is too slow.";

    // WARNING: This is O(layers) * O(markers), which is okay as long
    // as the number of layers with infomarks is small.
    //
    // If the performance gets too bad, count # in each layer,
    // allocate vectors, fill the vectors, and then only visit
    // each one once per layer.
    for (auto &it : result) {
        const int layer = it.first;
        InfomarksBatch batch{getOpenGL(), getGLFont()};
        for (const auto &m : m_data.getMarkersList()) {
            drawInfoMark(batch, m.get(), layer);
        }
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
    m_lines.emplace_back(m_color, a + m_offset);
    m_lines.emplace_back(m_color, b + m_offset);
}

void InfomarksBatch::drawTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
{
    m_tris.emplace_back(m_color, a + m_offset);
    m_tris.emplace_back(m_color, b + m_offset);
    m_tris.emplace_back(m_color, c + m_offset);
}

void InfomarksBatch::renderText(const glm::vec3 &pos,
                                const std::string &text,
                                const Color &color,
                                std::optional<Color> moved_bgcolor,
                                const FontFormatFlags fontFormatFlag,
                                const int rotationAngle)
{
    m_text.emplace_back(pos, text, color, std::move(moved_bgcolor), fontFormatFlag, rotationAngle);
}

InfomarksMeshes InfomarksBatch::getMeshes()
{
    InfomarksMeshes result;

    auto &gl = m_realGL;
    result.points = gl.createPointBatch(m_points);
    result.lines = gl.createColoredLineBatch(m_lines);
    result.tris = gl.createColoredTriBatch(m_tris);
    result.textMesh = m_font.getFontMesh(m_text);
    result.isValid = true;

    return result;
}

void InfomarksBatch::renderImmediate(const GLRenderState &state)
{
    auto &gl = m_realGL;

    if (!m_lines.empty())
        gl.renderColoredLines(m_lines, state);

    if (!m_tris.empty())
        gl.renderColoredTris(m_tris, state);

    if (!m_text.empty())
        m_font.render3dTextImmediate(m_text);

    if (!m_points.empty())
        gl.renderPoints(m_points, state.withPointSize(INFOMARK_POINT_SIZE));
}

void InfomarksMeshes::render()
{
    if (!isValid)
        return;

    const auto common_state
        = GLRenderState().withDepthFunction(std::nullopt).withBlend(BlendModeEnum::TRANSPARENCY);

    points.render(common_state.withPointSize(INFOMARK_POINT_SIZE));
    lines.render(common_state.withLineParams(LineParams{INFOMARK_ARROW_LINE_WIDTH}));
    tris.render(common_state);
    textMesh.render(common_state);
}

void MapCanvas::drawInfoMark(InfomarksBatch &batch,
                             InfoMark *const marker,
                             const int currentLayer,
                             const glm::vec2 &offset,
                             const std::optional<Color> &overrideColor)
{
    if (marker == nullptr) {
        assert(false);
        return;
    }

    const int layer = marker->getPosition1().z;
    if (layer != currentLayer) {
        // REVISIT: consider storing infomarks by level
        // so we don't have to test here.
        return;
    }

    const float x1 = static_cast<float>(marker->getPosition1().x) / INFOMARK_SCALE + offset.x;
    const float y1 = static_cast<float>(marker->getPosition1().y) / INFOMARK_SCALE + offset.y;
    const float x2 = static_cast<float>(marker->getPosition2().x) / INFOMARK_SCALE + offset.x;
    const float y2 = static_cast<float>(marker->getPosition2().y) / INFOMARK_SCALE + offset.y;
    const float dx = x2 - x1;
    const float dy = y2 - y1;

    const auto infoMarkType = marker->getType();
    const auto infoMarkClass = marker->getClass();

    // Color depends of the class of the InfoMark
    const Color infoMarkColor = getInfoMarkColor(infoMarkType, infoMarkClass).withAlpha(0.55f);
    const auto fontFormatFlag = getFontFormatFlags(infoMarkClass);

    const glm::vec3 pos{x1, y1, static_cast<float>(layer)};
    batch.setOffset(pos);

    const Color &color = (overrideColor) ? overrideColor.value() : infoMarkColor;
    batch.setColor(color);

    switch (infoMarkType) {
    case InfoMarkTypeEnum::TEXT: {
        batch.renderText(pos,
                         marker->getText().getStdString(),
                         Color{color.getQColor()},
                         color,
                         fontFormatFlag,
                         marker->getRotationAngle());
        break;
    }

    case InfoMarkTypeEnum::LINE:
        batch.drawLine(glm::vec3{0.f}, glm::vec3{dx, dy, 0.f});
        break;

    case InfoMarkTypeEnum::ARROW:
        batch.drawLineStrip(glm::vec3{0.f}, glm::vec3{dx - 0.3f, dy, 0.f}, glm::vec3{dx, dy, 0.f});
        batch.drawTriangle(glm::vec3{dx - 0.2f, dy + 0.07f, 0.f},
                           glm::vec3{dx - 0.2f, dy - 0.07f, 0.f},
                           glm::vec3{dx, dy, 0.f});
        break;
    }
}

void MapCanvas::paintNewInfomarkSelection()
{
    if (!hasSel1() || !hasSel2())
        return;

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

void MapCanvas::paintSelectedInfoMarks()
{
    if (m_infoMarkSelection == nullptr && m_canvasMouseMode != CanvasMouseModeEnum::SELECT_INFOMARKS)
        return;

    InfomarksBatch batch{getOpenGL(), getGLFont()};
    {
        // draw selections
        if (m_infoMarkSelection != nullptr) {
            for (const auto &marker : *m_infoMarkSelection) {
                drawInfoMark(batch, marker.get(), m_currentLayer, {}, Colors::red);
            }
            if (hasInfoMarkSelectionMove()) {
                const glm::vec2 offset = m_infoMarkSelectionMove->pos.to_vec2();
                for (const auto &marker : *m_infoMarkSelection) {
                    drawInfoMark(batch, marker.get(), m_currentLayer, offset, Colors::yellow);
                }
            }
        }

        // draw selection points
        if (m_canvasMouseMode == CanvasMouseModeEnum::SELECT_INFOMARKS) {
            const auto drawPoint = [&batch](const Coordinate &c, const Color &color) {
                batch.setColor(color);
                batch.setOffset(glm::vec3{0});
                const glm::vec3 point{static_cast<glm::vec2>(c.to_vec3())
                                          / static_cast<float>(INFOMARK_SCALE),
                                      c.z};
                batch.drawPoint(point);
            };

            const auto drawSelectionPoints = [this, &drawPoint](InfoMark *const marker) {
                const auto &pos1 = marker->getPosition1();
                if (pos1.z != m_currentLayer)
                    return;

                const auto color = (m_infoMarkSelection != nullptr
                                    && m_infoMarkSelection->contains(marker))
                                       ? Colors::yellow
                                       : Colors::cyan;

                drawPoint(pos1, color);
                if (marker->getType() == InfoMarkTypeEnum::TEXT)
                    return;

                const Coordinate &pos2 = marker->getPosition2();
                assert(pos2.z == m_currentLayer);
                drawPoint(pos2, color);
            };

            for (const auto &marker : m_data.getMarkersList()) {
                drawSelectionPoints(marker.get());
            }
        }
    }
    batch.renderImmediate(GLRenderState());
}

void MapCanvas::paintBatchedInfomarks()
{
    const auto wantInfoMarks = (getTotalScaleFactor() >= getConfig().canvas.infomarkScaleCutoff);
    if (!wantInfoMarks || !m_batches.infomarksMeshes.has_value()) {
        return;
    }

    BatchedInfomarksMeshes &map = m_batches.infomarksMeshes.value();
    const auto it = map.find(static_cast<int>(m_currentLayer));
    if (it == map.end())
        return;

    InfomarksMeshes &infomarksMeshes = it->second;
    infomarksMeshes.render();
}

void MapCanvas::updateInfomarkBatches()
{
    std::optional<BatchedInfomarksMeshes> &opt_infomarks = m_batches.infomarksMeshes;
    if (opt_infomarks.has_value())
        return;

    opt_infomarks.emplace(getInfoMarksMeshes());
}
