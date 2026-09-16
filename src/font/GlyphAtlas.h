#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <QtCore/QHash>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtGui/QImage>

class QRawFont;

namespace font {

struct NODISCARD AtlasGlyph final
{
    // atlas texel rect, upper left origin; includes PADDING on every side
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    // quad offset from the pen: x to the right, y UP from the baseline
    int xoffset = 0;
    int yoffset = 0;
    // color glyphs (emoji) are straight RGBA; otherwise RGB is white and A is coverage
    bool isColor = false;
    // no ink (e.g. space), or didn't fit; don't draw
    bool isEmpty = true;
};

struct NODISCARD ShapedGlyph final
{
    AtlasGlyph glyph;
    int x = 0; // pen offset from the start of the string
};

struct NODISCARD ShapedText final
{
    std::vector<ShapedGlyph> glyphs;
    int advance = 0;
};

// atlas pixels the GL texture doesn't have yet
struct NODISCARD AtlasUpload final
{
    QImage image; // RGBA8888, straight alpha; a copy of `rect`
    QRect rect;
    // the atlas grew, so the texture must be reallocated (rect covers everything)
    bool resized = false;
    int atlasWidth = 0;
    int atlasHeight = 0;
};

// Glyphs are shaped with QTextLayout and rasterized on demand into a
// shelf-packed RGBA8 image that doubles as needed (up to maxSize).
//
// NOTE: The cache is logically const, so every method is const. Batch
// generation calls shape() from worker threads; the GL thread calls
// takePendingUpload(). Everything mutable is guarded by m_mutex.
class NODISCARD GlyphAtlas final
{
public:
    // transparent texels around each glyph, so linear filtering can't bleed
    static constexpr int PADDING = 1;
    static constexpr int MIN_SIZE = 256;

    struct NODISCARD Config final
    {
        QFont font;
        int initialSize = MIN_SIZE;
        // NOTE: ES 3.0 / WebGL 2 only guarantee GL_MAX_TEXTURE_SIZE >= 2048.
        int maxSize = 2048;
    };

private:
    struct NODISCARD Shelf final
    {
        int x = 0;
        int y = 0;
        int height = 0;
    };

    mutable std::mutex m_mutex;
    const QFont m_font;
    const int m_maxSize;
    int m_lineHeight = 0;
    int m_ascent = 0;

    mutable QImage m_image;
    mutable Shelf m_shelf;
    mutable bool m_full = false;

    struct NODISCARD FontInfo final
    {
        uint32_t id = 0;
        bool isColorFont = false;
    };
    mutable QHash<QString, FontInfo> m_fonts; // family|style (fallback fonts)
    mutable std::unordered_map<uint64_t, AtlasGlyph> m_glyphs;
    mutable std::unordered_map<std::string, ShapedText> m_shaped;

    AtlasGlyph m_background;
    AtlasGlyph m_underline;

    mutable QRect m_dirty;
    mutable bool m_resized = false;

public:
    explicit GlyphAtlas(const Config &config);
    ~GlyphAtlas();
    DELETE_CTORS_AND_ASSIGN_OPS(GlyphAtlas);

public:
    NODISCARD const QFont &getFont() const { return m_font; }
    NODISCARD int getLineHeight() const { return m_lineHeight; }
    NODISCARD int getAscent() const { return m_ascent; }
    NODISCARD int getMaxSize() const { return m_maxSize; }
    // solid white blocks, so background and underline draw in the same batch
    NODISCARD const AtlasGlyph &getBackground() const { return m_background; }
    NODISCARD const AtlasGlyph &getUnderline() const { return m_underline; }

public:
    // single line; rasterizes any glyphs not already in the atlas
    NODISCARD ShapedText shape(std::string_view utf8) const;
    NODISCARD int getAdvance(std::string_view utf8) const { return shape(utf8).advance; }

public:
    // GL thread: what changed since the last call, if anything
    NODISCARD std::optional<AtlasUpload> takePendingUpload() const;
    NODISCARD QSize getSize() const;
    // hit maxSize; new glyphs are being dropped
    NODISCARD bool isFull() const;
    NODISCARD QImage copyImage() const;
    NODISCARD size_t getNumGlyphs() const;

private:
    // caller must hold m_mutex
    NODISCARD ShapedText shapeLocked(std::string_view utf8) const;
    NODISCARD const AtlasGlyph &lookupOrRasterizeLocked(const QRawFont &rawFont,
                                                        uint32_t glyphIndex) const;
    NODISCARD AtlasGlyph rasterizeLocked(const QRawFont &rawFont,
                                         uint32_t glyphIndex,
                                         bool isColorFont) const;
    NODISCARD std::optional<QPoint> allocateLocked(int width, int height) const;
    NODISCARD bool growLocked() const;
    void markDirtyLocked(const QRect &rect) const;
};

NODISCARD int pointSizeToPhysicalPixels(int pointSize, float logicalDpi, float devicePixelRatio);

} // namespace font
