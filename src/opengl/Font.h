#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Color.h"
#include "../global/RuleOf5.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"
#include "OpenGL.h"
#include "OpenGLTypes.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include <QString>

struct MapCanvasViewport;

struct NODISCARD GLText final
{
    glm::vec3 pos{0.f};
    std::string text;
    Color color;
    std::optional<Color> bgcolor;
    FontFormatFlags fontFormatFlag;
    int rotationAngle = 0;

    explicit GLText(const glm::vec3 &pos_,
                    std::string moved_text,
                    const Color &color_ = {},
                    std::optional<Color> bgcolor_ = {},
                    const FontFormatFlags &fontFormatFlag_ = {},
                    int rotationAngle_ = 0)
        : pos{pos_}
        , text{std::move(moved_text)}
        , color{color_}
        , bgcolor{bgcolor_}
        , fontFormatFlag{fontFormatFlag_}
        , rotationAngle{rotationAngle_}
    {}
};

struct FontMetrics;

class NODISCARD GLFont final
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
    NODISCARD int getFontHeight() const;
    NODISCARD std::optional<int> getGlyphAdvance(char c) const;

private:
    NODISCARD glm::ivec2 getScreenCenter() const;

public:
    void renderTextCentered(const QString &text,
                            const Color &color = {},
                            const std::optional<Color> &bgcolor = {});
    void render2dTextImmediate(const std::vector<GLText> &text);
    void render3dTextImmediate(const std::vector<GLText> &text);

public:
    NODISCARD UniqueMesh getFontMesh(const std::vector<GLText> &text);

private:
    NODISCARD std::vector<FontVert3d> getFontBatchRawData(const GLText *text, size_t count);
};
