#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstddef>
#include <glm/glm.hpp>
#include <optional>
#include <unordered_map>
#include <vector>
#include <QString>

#include "../global/Color.h"
#include "../global/utils.h"
#include "../opengl/Font.h"
#include "../opengl/FontFormatFlags.h"
#include "../opengl/OpenGLTypes.h"

class OpenGL;

struct NODISCARD InfomarksMeshes final
{
    UniqueMesh points;
    UniqueMesh lines;
    UniqueMesh tris;
    UniqueMesh textMesh;
    bool isValid = false;
    void render();
};

using BatchedInfomarksMeshes = std::unordered_map<int, InfomarksMeshes>;

struct NODISCARD InfomarksBatch final
{
private:
    OpenGL &m_realGL;
    GLFont &m_font;
    glm::vec3 m_offset{0};
    Color m_color;

    std::vector<ColorVert> m_points;
    std::vector<ColorVert> m_lines;
    std::vector<ColorVert> m_tris;
    std::vector<GLText> m_text;

public:
    explicit InfomarksBatch(OpenGL &gl, GLFont &font)
        : m_realGL{gl}
        , m_font{font}
    {}

    void setColor(const Color &color) { m_color = color; }
    void setOffset(const glm::vec3 &offset) { m_offset = offset; }
    void drawPoint(const glm::vec3 &a);
    void drawLine(const glm::vec3 &a, const glm::vec3 &b);
    void drawLineStrip(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
    {
        drawLine(a, b);
        drawLine(b, c);
    }
    void drawTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);
    void renderText(const glm::vec3 &pos,
                    const std::string &text,
                    const Color &color,
                    std::optional<Color> moved_bgcolor,
                    const FontFormatFlags &fontFormatFlag,
                    int rotationAngle);

    NODISCARD InfomarksMeshes getMeshes();
    void renderImmediate(const GLRenderState &state);
};
