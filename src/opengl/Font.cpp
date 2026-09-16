// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

// FIXME: including display/ from opengl/ is a modularity violation.

#include "Font.h"

#include "../configuration/configuration.h"
#include "../display/Textures.h"
#include "../font/GlyphAtlas.h"
#include "../global/ConfigConsts.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"
#include "OpenGL.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QtCore>
#include <QtGui>

// NOTE: Rect doesn't actually include the hi value.
struct NODISCARD Rect final
{
    glm::ivec2 lo = glm::ivec2{0};
    glm::ivec2 hi = glm::ivec2{0};

    NODISCARD int width() const { return hi.x - lo.x; }
    NODISCARD int height() const { return hi.y - lo.y; }
    NODISCARD glm::ivec2 size() const { return {width(), height()}; }
};

// NOTE: shared with the worker threads that generate text meshes.
struct NODISCARD FontMetrics final
{
    struct NODISCARD Common final
    {
        int lineHeight = 0;
        int base = 0;
        int marginX = 2;
        int marginY = 1;
    };

    font::GlyphAtlas atlas;
    Common common;

    explicit FontMetrics(const font::GlyphAtlas::Config &config)
        : atlas{config}
    {
        common.lineHeight = atlas.getLineHeight();
        common.base = atlas.getAscent();
    }

    void getFontBatchRawData(const GLText *text,
                             size_t count,
                             std::vector<FontVert3d> &output) const;
};

void getFontBatchRawData(const FontMetrics &fm,
                         const GLText *const text,
                         const size_t count,
                         std::vector<FontVert3d> &output)
{
    fm.getFontBatchRawData(text, count, output);
}

class NODISCARD FontBatchBuilder final
{
private:
    // only valid as long as GLText it refers to is valid.
    struct NODISCARD Opts final
    {
        std::string_view msg;
        glm::vec3 pos{0.f};
        Color fgColor;
        std::optional<Color> optBgColor;
        bool wantItalics = false;
        bool wantUnderline = false;
        bool wantAlignCenter = false;
        bool wantAlignRight = false;
        int rotationDegrees = 0;
        std::optional<glm::mat4> rotation = glm::mat4(1.f);

        explicit Opts() = default;

        void reset() { *this = Opts{}; }

        explicit Opts(const GLText &text)
            : msg{text.text}
            , pos{text.pos}
            , fgColor{text.color}
            , optBgColor{text.bgcolor}
            , wantItalics{text.fontFormatFlag.contains(FontFormatFlagEnum::ITALICS)}
            , wantUnderline{text.fontFormatFlag.contains(FontFormatFlagEnum::UNDERLINE)}
            , wantAlignCenter{text.fontFormatFlag.contains(FontFormatFlagEnum::HALIGN_CENTER)}
            , wantAlignRight{text.fontFormatFlag.contains(FontFormatFlagEnum::HALIGN_RIGHT)}
            , rotationDegrees{text.rotationAngle}
            , rotation{(rotationDegrees == 0)
                           ? std::optional<glm::mat4>(std::nullopt)
                           : std::optional<glm::mat4>(
                                 glm::rotate(glm::mat4(1),
                                             glm::radians(static_cast<float>(rotationDegrees)),
                                             glm::vec3(0, 0, 1)))}
        {}
    };

    struct NODISCARD Bounds final
    {
        glm::ivec2 maxVertPos{};
        glm::ivec2 minVertPos{};
        void include(const glm::ivec2 vertPos)
        {
            minVertPos = glm::min(vertPos, minVertPos);
            maxVertPos = glm::max(vertPos, maxVertPos);
        }
    };

private:
    const FontMetrics &m_fm;
    std::vector<FontVert3d> &m_verts3d;
    Opts m_opts;
    Bounds m_bounds;
    font::ShapedText m_shaped;
    int m_xlinepos = 0;
    bool m_noOutput = false;
    void resetPerStringData()
    {
        m_opts.reset();
        m_bounds = Bounds();
        m_shaped = font::ShapedText{};
        m_xlinepos = 0;
        m_noOutput = false;
    }

public:
    explicit FontBatchBuilder(const FontMetrics &fm, std::vector<FontVert3d> &output)
        : m_fm{fm}
        , m_verts3d{output}
    {}

    // NOTE: tex coords are atlas texels (upper left origin); the vertex shader
    // divides by textureSize(), so the atlas can grow under existing meshes.
    NODISCARD static glm::vec2 getTexCoord(const glm::ivec2 iTexCoord)
    {
        return glm::vec2(iTexCoord);
    }

    // REVISIT: This could be done in the shader,
    // at the cost of transmitting italics bit and rotation angle.
    NODISCARD glm::vec2 transformVert(const glm::ivec2 ipos) const
    {
        glm::vec2 pos(ipos);

        if (m_opts.wantItalics) {
            pos.x += pos.y / 6.f;
        }

        if (m_opts.rotation) {
            pos = glm::vec2(m_opts.rotation.value() * glm::vec4(pos, 0, 1));
        }
        return pos;
    }

    void emitGlyphQuad(const glm::ivec2 iVertex00, const font::AtlasGlyph &g)
    {
        const auto emitWithOffset = [this, &iVertex00, &g](const glm::ivec2 pixelOffset) -> void {
            const glm::ivec2 relativeVertPos = iVertex00 + pixelOffset;
            // side-effect: updates bounds; this must come before return
            m_bounds.include(relativeVertPos);

            if (m_noOutput) {
                return;
            }

            // vertex y goes up, but atlas rows go down
            const glm::ivec2 iTexCoord{g.x + pixelOffset.x, g.y + (g.height - pixelOffset.y)};
            const glm::vec2 tc = getTexCoord(iTexCoord);
            const glm::vec2 vert = transformVert(relativeVertPos);
            m_verts3d.emplace_back(m_opts.pos, m_opts.fgColor, tc, vert, g.isColor ? 1.f : 0.f);
        };

        const auto &x = g.width;
        const auto &y = g.height;
        // 3-2
        // | |
        // 0-1
        emitWithOffset(glm::ivec2(0, 0));
        emitWithOffset(glm::ivec2(x, 0));
        emitWithOffset(glm::ivec2(x, y));
        emitWithOffset(glm::ivec2(0, y));
    }

    void emitGlyphs(const int wordOffset, const bool output)
    {
        m_noOutput = !output;
        for (const font::ShapedGlyph &sg : m_shaped.glyphs) {
            const font::AtlasGlyph &g = sg.glyph;
            if (g.isEmpty) {
                continue;
            }
            const glm::ivec2 iVertex00{wordOffset + sg.x + g.xoffset, g.yoffset};
            emitGlyphQuad(iVertex00, g);
        }
        m_xlinepos = wordOffset + m_shaped.advance;
    }

    void addString(const GLText &text)
    {
        resetPerStringData();
        this->m_opts = Opts{text};
        m_shaped = m_fm.atlas.shape(m_opts.msg);

        int wordOffset = 0;
        emitGlyphs(wordOffset, false);

        // measurement, background color, and underline.
        {
            const auto add = [this](const Color c, const glm::ivec2 ivert, const glm::ivec2 itc) {
                const glm::vec2 tc = getTexCoord(itc);
                const glm::vec2 vert = transformVert(ivert);
                m_verts3d.emplace_back(m_opts.pos, c, tc, vert);
            };

            const auto quad = [&add](const Color c, const Rect &vert, const Rect &tc) {
#define ADD(a, b) add(c, glm::ivec2{vert.a.x, vert.b.y}, glm::ivec2{tc.a.x, tc.b.y})
                // note: lo and hi refer to members of vert and tc.
                ADD(lo, lo);
                ADD(hi, lo);
                ADD(hi, hi);
                ADD(lo, hi);
#undef ADD
            };

            const auto texRect = [](const font::AtlasGlyph &g) -> Rect {
                const glm::ivec2 lo{g.x, g.y};
                return Rect{lo, lo + glm::ivec2{g.width, g.height}};
            };

            const glm::ivec2 margin{m_fm.common.marginX, m_fm.common.marginY};
            const auto &lo = m_bounds.minVertPos;
            const auto &hi = m_bounds.maxVertPos;

            if (m_opts.wantAlignCenter) {
                const auto halfWidth = m_xlinepos / 2;
                wordOffset -= halfWidth;
                m_bounds.minVertPos.x -= halfWidth;
                m_bounds.maxVertPos.x -= halfWidth;
            } else if (m_opts.wantAlignRight) {
                wordOffset -= m_xlinepos;
                m_bounds.minVertPos.x -= m_xlinepos;
                m_bounds.maxVertPos.x -= m_xlinepos;
            }

            if (m_opts.optBgColor) {
                // bounds include the glyph padding; shrink back to the ink
                const glm::ivec2 pad{font::GlyphAtlas::PADDING};
                quad(m_opts.optBgColor.value(),
                     Rect{lo + pad - margin, hi - pad + margin},
                     texRect(m_fm.atlas.getBackground()));
            }

            if (m_opts.wantUnderline) {
                const font::AtlasGlyph &underline = m_fm.atlas.getUnderline();
                const glm::ivec2 offset{wordOffset + underline.xoffset, underline.yoffset};
                quad(m_opts.fgColor,
                     Rect{offset, offset + glm::ivec2{m_xlinepos, underline.height}},
                     texRect(underline));
            }
        }

        // note: 2nd call includes wordOffset that can be modified
        // above if caller requested HALIGN_CENTER or HALIGN_RIGHT.
        emitGlyphs(wordOffset, true);
    }
};

void FontMetrics::getFontBatchRawData(const GLText *const text,
                                      const size_t count,
                                      std::vector<FontVert3d> &output) const
{
    if (count == 0) {
        return;
    }

    const auto before = output.size();
    const auto end = text + count;

    // upper bound: utf8 byte count >= glyph count
    const size_t maxExpectedVerts = std::invoke([text, end]() -> size_t {
        size_t numQuads = 0;
        for (const GLText *it = text; it != end; ++it) {
            numQuads += it->text.size() + (it->bgcolor.has_value() ? 1 : 0)
                        + (it->fontFormatFlag.contains(FontFormatFlagEnum::UNDERLINE) ? 1 : 0);
        }
        return 4 * numQuads;
    });

    output.reserve(before + maxExpectedVerts);

    FontBatchBuilder fontBatchBuilder{*this, output};
    for (const GLText *it = text; it != end; ++it) {
        fontBatchBuilder.addString(*it);
    }
    assert(output.size() <= before + maxExpectedVerts);
}

GLFont::GLFont(OpenGL &gl)
    : m_gl(gl)
{}

GLFont::~GLFont() = default;

NODISCARD static int getMaxTextureSize()
{
    // NOTE: ES 3.0 / WebGL 2 only guarantee 2048; that's ~10k glyphs (16 MiB).
    constexpr GLint DESIRED = 2048;
    GLint maxSize = 0;
    if (QOpenGLContext *const ctx = QOpenGLContext::currentContext()) {
        ctx->functions()->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    }
    if (maxSize <= 0) {
        return DESIRED;
    }
    return std::min(maxSize, DESIRED);
}

void GLFont::init(const float logicalDpi)
{
    assert(m_gl.isRendererInitialized());

    const auto &canvas = getConfig().canvas;
    const QString family = canvas.mapFontFamily.get();
    const int pointSize = canvas.mapFontPointSize.get();
    const float dpr = m_gl.getDevicePixelRatio();
    m_logicalDpi = logicalDpi;

    // the font shader works in physical pixels, so rasterize at that size
    const int pixelSize = font::pointSizeToPhysicalPixels(pointSize, logicalDpi, dpr);
    QFont qfont(family);
    qfont.setPixelSize(pixelSize);
    // NOTE: Cantarell has no symbols or emoji. Prefer the platform's color
    // emoji font (whichever exists); the bundled DejaVu Sans Mono then
    // guarantees the monochrome symbol blocks (arrows, U+2600, dingbats).
    qfont.setFamilies({family,
                       QStringLiteral("Apple Color Emoji"),
                       QStringLiteral("Segoe UI Emoji"),
                       QStringLiteral("Noto Color Emoji"),
                       QStringLiteral("DejaVu Sans Mono")});

    font::GlyphAtlas::Config config;
    config.font = qfont;
    config.maxSize = getMaxTextureSize();

    qInfo() << "Map font:" << family << pointSize << "pt =" << pixelSize << "px (dpi" << logicalDpi
            << ", dpr" << dpr << ")";

    m_fontMetrics = std::make_shared<FontMetrics>(config);

    if (m_texture) {
        m_texture->clearId();
    }

    // NOTE: created empty; syncTexture() allocates and later grows it in place,
    // so meshes holding this MMTexture stay valid.
    m_texture = MMTexture::alloc(
        QOpenGLTexture::Target::Target2D,
        [](QOpenGLTexture &tex) -> void {
            tex.setMinMagFilters(QOpenGLTexture::Filter::Linear, QOpenGLTexture::Filter::Linear);
            tex.setWrapMode(QOpenGLTexture::ClampToEdge);
            tex.setAutoMipMapGenerationEnabled(false);
        },
        true);
    m_texture->setId(m_id);
    m_gl.setTextureLookup(m_id, m_texture);

    syncTexture();
}

void GLFont::syncTexture()
{
    if (!m_fontMetrics || !m_texture) {
        return;
    }

    const std::optional<font::AtlasUpload> upload = m_fontMetrics->atlas.takePendingUpload();
    if (!upload) {
        return;
    }

    QOpenGLTexture &tex = deref(m_texture->get());
    const QImage &img = upload->image;
    assert(img.format() == QImage::Format_RGBA8888);

    if (upload->resized || !tex.isStorageAllocated()) {
        if (tex.isCreated()) {
            tex.destroy();
        }
        // RGBA8888 is byte-ordered (endian independent) and straight alpha
        tex.setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
        tex.setMipLevels(1);
        tex.setSize(upload->atlasWidth, upload->atlasHeight);
        tex.allocateStorage(QOpenGLTexture::PixelFormat::RGBA, QOpenGLTexture::PixelType::UInt8);
        tex.setMinMagFilters(QOpenGLTexture::Filter::Linear, QOpenGLTexture::Filter::Linear);
        tex.setWrapMode(QOpenGLTexture::ClampToEdge);
        tex.setAutoMipMapGenerationEnabled(false);
        if (!tex.isStorageAllocated()) {
            qWarning() << "Unable to allocate" << upload->atlasWidth << "x" << upload->atlasHeight
                       << "font atlas texture";
            return;
        }
    }

    assert(img.width() == upload->rect.width() && img.height() == upload->rect.height());
    tex.setData(upload->rect.x(),
                upload->rect.y(),
                0,
                upload->rect.width(),
                upload->rect.height(),
                1,
                QOpenGLTexture::PixelFormat::RGBA,
                QOpenGLTexture::PixelType::UInt8,
                img.constBits());
}

void GLFont::cleanup()
{
    m_fontMetrics.reset();
    m_texture.reset();
    m_logicalDpi = 0.f;
}

int GLFont::getFontHeight() const
{
    return getFontMetrics().common.lineHeight;
}

std::optional<int> GLFont::getGlyphAdvance(const char c) const
{
    if (c == '\0') {
        return std::nullopt;
    }
    const auto shaped = getFontMetrics().atlas.shape(std::string_view{&c, 1});
    if (shaped.glyphs.empty()) {
        return std::nullopt;
    }
    return shaped.advance;
}

glm::ivec2 GLFont::getScreenCenter() const
{
    return m_gl.getPhysicalViewport().offset + m_gl.getPhysicalViewport().size / 2;
}

void GLFont::render2dTextImmediate(const View<GLText> text)
{
    if (text.empty()) {
        return;
    }

    // input position: physical pixels, with origin at upper left
    // output: [-1, 1]^2
    const auto vp = m_gl.getPhysicalViewport();
    const auto viewProj = glm::scale(glm::mat4(1), glm::vec3(2, 2, 1))
                          * glm::translate(glm::mat4(1), glm::vec3(-0.5f, 0.5, 0))
                          * glm::scale(glm::mat4(1),
                                       glm::vec3(1.f / glm::vec2(vp.size.x, -vp.size.y), 1.f))
                          * glm::translate(glm::mat4(1), glm::vec3(-glm::vec2(vp.offset), 1.f));

    const auto oldProj = m_gl.getProjectionMatrix();
    m_gl.setProjectionMatrix(viewProj);
    render3dTextImmediate(text);
    m_gl.setProjectionMatrix(oldProj);
}

void GLFont::render3dTextImmediate(const View<FontVert3d> rawVerts)
{
    if (rawVerts.empty()) {
        return;
    }

    syncTexture();
    m_gl.renderFont3d(m_texture, rawVerts);
}

void GLFont::render3dTextImmediate(const View<GLText> text)
{
    if (text.empty()) {
        return;
    }

    const auto rawVerts = getFontMeshIntermediate(text);
    render3dTextImmediate(rawVerts);
}

std::vector<FontVert3d> GLFont::getFontMeshIntermediate(const View<GLText> text)
{
    std::vector<FontVert3d> output;
    getFontMetrics().getFontBatchRawData(text.data(), text.size(), output);
    return output;
}

UniqueMesh GLFont::getFontMesh(const View<FontVert3d> rawVerts)
{
    // the verts may reference glyphs added by a worker thread
    syncTexture();
    return m_gl.createFontMesh(m_texture, DrawModeEnum::QUADS, rawVerts);
}

void GLFont::renderTextCentered(const QString &text,
                                const Color color,
                                const std::optional<Color> bgcolor)
{
    const auto center = glm::vec2{getScreenCenter()};
    render2dTextImmediate(
        std::vector<GLText>{GLText{glm::vec3{center, 0.f},
                                   mmqt::toStdStringUtf8(text),
                                   color,
                                   bgcolor,
                                   FontFormatFlags{FontFormatFlagEnum::HALIGN_CENTER}}});
}
