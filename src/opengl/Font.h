#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstddef>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>
#include <QString>

#include "../global/Color.h"
#include "../global/RuleOf5.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"
#include "OpenGL.h"
#include "OpenGLTypes.h"

struct MapCanvasViewport;

struct NODISCARD GLText final
{
    glm::vec3 pos{0.f};
    std::string text;
    Color color;
    std::optional<Color> bgcolor;
    FontFormatFlags fontFormatFlag;
    int rotationAngle = 0;

    explicit GLText(const glm::vec3 &pos,
                    const QString &text,
                    const Color &color = {},
                    std::optional<Color> moved_bgcolor = {},
                    const FontFormatFlags &fontFormatFlag = {},
                    int rotationAngle = 0);

    explicit GLText(const glm::vec3 &pos,
                    std::string moved_text,
                    const Color &color = {},
                    std::optional<Color> moved_bgcolor = {},
                    const FontFormatFlags &fontFormatFlag = {},
                    int rotationAngle = 0)
        : pos(pos)
        , text(std::move(moved_text))
        , color(color)
        , bgcolor(std::move(moved_bgcolor))
        , fontFormatFlag(fontFormatFlag)
        , rotationAngle(rotationAngle)
    {}
};

struct FontMetrics;

class GLFont final
{
private:
    OpenGL &m_gl;
    SharedMMTexture m_texture;
    std::unique_ptr<FontMetrics> m_fontMetrics;

public:
    explicit GLFont(OpenGL &gl);
    ~GLFont();
    DELETE_CTORS_AND_ASSIGN_OPS(GLFont);

private:
    const FontMetrics &getFontMetrics() const { return deref(m_fontMetrics); }

public:
    void init();
    void cleanup();

public:
    int getFontHeight() const;
    std::optional<int> getGlyphAdvance(char c) const;

private:
    glm::ivec2 getScreenCenter() const;

public:
    void renderTextCentered(const QString &text,
                            const Color &color = {},
                            const std::optional<Color> &bgcolor = {});
    void render2dTextImmediate(const std::vector<GLText> &text);
    void render3dTextImmediate(const std::vector<GLText> &text);

public:
    UniqueMesh getFontMesh(const std::vector<GLText> &text);

private:
    std::vector<FontVert3d> getFontBatchRawData(const GLText *text, size_t count);
};
