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
#include <tuple>
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
                    const Color color_ = {},
                    std::optional<Color> bgcolor_ = {},
                    const FontFormatFlags fontFormatFlag_ = {},
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
    MMTextureId m_id = INVALID_MM_TEXTURE_ID;
    std::shared_ptr<const FontMetrics> m_fontMetrics;

public:
    explicit GLFont(OpenGL &gl);
    ~GLFont();
    DELETE_CTORS_AND_ASSIGN_OPS(GLFont);

private:
    NODISCARD const FontMetrics &getFontMetrics() const { return deref(m_fontMetrics); }

public:
    NODISCARD std::shared_ptr<const FontMetrics> getSharedFontMetrics() const
    {
        std::ignore = deref(m_fontMetrics); // throws if nullptr
        return m_fontMetrics;
    }

public:
    void setTextureId(const MMTextureId id)
    {
        assert(m_id == INVALID_MM_TEXTURE_ID);
        m_id = id;
    }
    void init();
    void cleanup();

public:
    NODISCARD int getFontHeight() const;
    NODISCARD std::optional<int> getGlyphAdvance(char c) const;

private:
    NODISCARD glm::ivec2 getScreenCenter() const;

public:
    void renderTextCentered(const QString &text,
                            Color color = {},
                            std::optional<Color> bgcolor = {});
    void render2dTextImmediate(const std::vector<GLText> &text);
    void render3dTextImmediate(const std::vector<GLText> &text);
    void render3dTextImmediate(const std::vector<FontVert3d> &rawVerts);

public:
    NODISCARD std::vector<FontVert3d> getFontMeshIntermediate(const std::vector<GLText> &text);
    NODISCARD UniqueMesh getFontMesh(const std::vector<FontVert3d> &text);
};

extern void getFontBatchRawData(const FontMetrics &fm,
                                const GLText *text,
                                size_t count,
                                std::vector<FontVert3d> &output);
