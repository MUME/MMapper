// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "GlyphAtlas.h"

#include "../global/utils.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <QtCore/QDebug>
#include <QtGui/QFontMetrics>
#include <QtGui/QGlyphRun>
#include <QtGui/QPainter>
#include <QtGui/QRawFont>
#include <QtGui/QTextLayout>

namespace font {

namespace {

// bounds the shaped-string memo; re-shaping is cheap, glyphs stay cached
constexpr size_t MAX_SHAPED_STRINGS = 1u << 16;

// solid white block at the atlas origin for the background and underline "glyphs"
constexpr int WHITE_BLOCK_SIZE = 4;

// same test as FreeType's FT_HAS_COLOR
NODISCARD bool hasColorTables(const QRawFont &rawFont)
{
    for (const char *const table : {"CBDT", "sbix", "COLR", "SVG "}) {
        if (!rawFont.fontTable(table).isEmpty()) {
            return true;
        }
    }
    return false;
}

NODISCARD int roundUpPow2(int n)
{
    return static_cast<int>(utils::nextPowerOfTwo(static_cast<uint32_t>(std::max(1, n))));
}

} // namespace

int pointSizeToPhysicalPixels(const int pointSize,
                              const float logicalDpi,
                              const float devicePixelRatio)
{
    const float dpi = (logicalDpi > 0.f) ? logicalDpi : 96.f;
    const float dpr = (devicePixelRatio > 0.f) ? devicePixelRatio : 1.f;
    return std::max(1,
                    static_cast<int>(std::lround(static_cast<float>(pointSize) * dpi / 72.f * dpr)));
}

GlyphAtlas::GlyphAtlas(const Config &config)
    : m_font{std::invoke([&config]() {
        QFont f = config.font;
        // NOTE: glyphs are displayed 1:1, so hinting is pure win
        // (the pre-baked BMFont atlases were hinted, too).
        f.setHintingPreference(QFont::PreferFullHinting);
        f.setStyleStrategy(QFont::PreferAntialias);
        return f;
    })}
    , m_maxSize{std::max(MIN_SIZE, roundUpPow2(config.maxSize))}
{
    const QFontMetrics fm(m_font);
    m_lineHeight = std::max(1, fm.height());
    m_ascent = std::max(1, fm.ascent());

    const int initial = std::clamp(roundUpPow2(config.initialSize), MIN_SIZE, m_maxSize);
    m_image = QImage(initial, initial, QImage::Format_RGBA8888);
    m_image.fill(Qt::transparent);

    // white block goes first so its location is fixed
    for (int y = 0; y < WHITE_BLOCK_SIZE; ++y) {
        std::memset(m_image.scanLine(y), 0xFF, WHITE_BLOCK_SIZE * 4);
    }
    m_shelf = Shelf{WHITE_BLOCK_SIZE, 0, WHITE_BLOCK_SIZE};

    // interior of the block, so linear filtering at the quad edges stays white
    m_background = AtlasGlyph{1, 1, 2, 2, 0, 0, false, false};
    // 1px tall, just below the baseline
    m_underline = AtlasGlyph{1, 1, 2, 1, 0, -1, false, false};

    m_resized = true;
    m_dirty = m_image.rect();
}

GlyphAtlas::~GlyphAtlas() = default;

ShapedText GlyphAtlas::shape(const std::string_view utf8) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    std::string key{utf8};
    if (const auto it = m_shaped.find(key); it != m_shaped.end()) {
        return it->second;
    }

    ShapedText result = shapeLocked(utf8);
    if (m_shaped.size() >= MAX_SHAPED_STRINGS) {
        m_shaped.clear();
    }
    m_shaped.emplace(std::move(key), result);
    return result;
}

ShapedText GlyphAtlas::shapeLocked(const std::string_view utf8) const
{
    ShapedText result;
    if (utf8.empty()) {
        return result;
    }

    QString str = QString::fromUtf8(utf8.data(), static_cast<qsizetype>(utf8.size()));
    // single line only
    str.replace(QChar::LineFeed, QChar::Space);
    str.replace(QChar::CarriageReturn, QChar::Space);

    QTextLayout layout(str, m_font);
    QTextOption option;
    option.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(option);
    layout.setCacheEnabled(false);
    layout.beginLayout();
    const QTextLine line = layout.createLine();
    layout.endLayout();

    if (!line.isValid()) {
        return result;
    }

    result.advance = qRound(line.naturalTextWidth());

    // one run per (fallback) font
    for (const QGlyphRun &run : layout.glyphRuns()) {
        const QRawFont rawFont = run.rawFont();
        const QList<quint32> indexes = run.glyphIndexes();
        const QList<QPointF> positions = run.positions();
        const auto count = std::min(indexes.size(), positions.size());
        result.glyphs.reserve(result.glyphs.size() + static_cast<size_t>(count));
        for (qsizetype i = 0; i < count; ++i) {
            const AtlasGlyph &glyph = lookupOrRasterizeLocked(rawFont, indexes[i]);
            result.glyphs.push_back(ShapedGlyph{glyph, qRound(positions[i].x())});
        }
    }

    return result;
}

const AtlasGlyph &GlyphAtlas::lookupOrRasterizeLocked(const QRawFont &rawFont,
                                                      const uint32_t glyphIndex) const
{
    // glyph indexes are only unique within a font
    const QString fontKey = rawFont.familyName() + QChar('|') + rawFont.styleName();
    auto fontIt = m_fonts.find(fontKey);
    if (fontIt == m_fonts.end()) {
        fontIt = m_fonts.insert(fontKey,
                                FontInfo{static_cast<uint32_t>(m_fonts.size()),
                                         hasColorTables(rawFont)});
    }
    const FontInfo &info = fontIt.value();
    const uint64_t key = (static_cast<uint64_t>(info.id) << 32u) | glyphIndex;

    if (const auto it = m_glyphs.find(key); it != m_glyphs.end()) {
        return it->second;
    }
    return m_glyphs.emplace(key, rasterizeLocked(rawFont, glyphIndex, info.isColorFont))
        .first->second;
}

AtlasGlyph GlyphAtlas::rasterizeLocked(const QRawFont &rawFont,
                                       const uint32_t glyphIndex,
                                       const bool isColorFont) const
{
    AtlasGlyph empty;
    empty.isEmpty = true;

    const QRectF bbox = rawFont.boundingRect(glyphIndex);
    if (bbox.isEmpty()) {
        return empty; // e.g. space
    }

    // NOTE: hinting can push pixels outside the outline bbox,
    // so render with slack and crop to the actual ink.
    constexpr int SLACK = 2;
    const int left = static_cast<int>(std::floor(bbox.left())) - SLACK;
    const int top = static_cast<int>(std::floor(bbox.top())) - SLACK;
    const int w = static_cast<int>(std::ceil(bbox.right())) - left + SLACK;
    const int h = static_cast<int>(std::ceil(bbox.bottom())) - top + SLACK;
    if (w <= 0 || h <= 0) {
        return empty;
    }

    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    {
        QGlyphRun run;
        run.setRawFont(rawFont);
        run.setGlyphIndexes({glyphIndex});
        run.setPositions({QPointF(0, 0)});

        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        // mono glyphs come out premultiplied white (a,a,a,a);
        // color fonts ignore the pen
        painter.setPen(Qt::white);
        painter.drawGlyphRun(QPointF(-left, -top), run);
    }

    int minX = w;
    int minY = h;
    int maxX = -1;
    int maxY = -1;
    bool isColor = false;
    for (int y = 0; y < h; ++y) {
        const QRgb *const row = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            const QRgb px = row[x];
            const int a = qAlpha(px);
            if (a == 0) {
                continue;
            }
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
            // NOTE: color fonts still have monochrome glyphs (e.g. digits),
            // which render in the pen color and must stay tintable.
            if (isColorFont && (qRed(px) != a || qGreen(px) != a || qBlue(px) != a)) {
                isColor = true;
            }
        }
    }
    if (maxX < 0) {
        return empty;
    }

    const int inkW = maxX - minX + 1;
    const int inkH = maxY - minY + 1;
    const int cellW = inkW + 2 * PADDING;
    const int cellH = inkH + 2 * PADDING;

    const std::optional<QPoint> cell = allocateLocked(cellW, cellH);
    if (!cell) {
        return empty;
    }

    // straight alpha for BlendModeEnum::TRANSPARENCY
    const QImage ink = img.copy(minX, minY, inkW, inkH).convertToFormat(QImage::Format_RGBA8888);
    for (int y = 0; y < inkH; ++y) {
        const uchar *const src = ink.constScanLine(y);
        uchar *const dst = m_image.scanLine(cell->y() + PADDING + y) + (cell->x() + PADDING) * 4;
        std::memcpy(dst, src, static_cast<size_t>(inkW) * 4);
        if (!isColor) {
            // coverage in alpha; white RGB so the shader can tint
            for (int x = 0; x < inkW; ++x) {
                dst[x * 4 + 0] = 0xFF;
                dst[x * 4 + 1] = 0xFF;
                dst[x * 4 + 2] = 0xFF;
            }
        }
    }
    markDirtyLocked(QRect(*cell, QSize(cellW, cellH)));

    AtlasGlyph g;
    g.x = cell->x();
    g.y = cell->y();
    g.width = cellW;
    g.height = cellH;
    g.xoffset = (left + minX) - PADDING;
    // ink bottom is (top + maxY + 1) in y-down; flip to y-up
    g.yoffset = -(top + maxY + 1) - PADDING;
    g.isColor = isColor;
    g.isEmpty = false;
    return g;
}

std::optional<QPoint> GlyphAtlas::allocateLocked(const int width, const int height) const
{
    for (;;) {
        const int atlasW = m_image.width();
        const int atlasH = m_image.height();

        if (m_shelf.x + width > atlasW) {
            // start a new shelf
            m_shelf.y += m_shelf.height;
            m_shelf.x = 0;
            m_shelf.height = 0;
        }

        if (m_shelf.y + height > atlasH || width > atlasW) {
            if (!growLocked()) {
                if (!m_full) {
                    m_full = true;
                    qWarning() << "Font atlas is full at" << atlasW << "x" << atlasH
                               << "; further new glyphs will not be displayed.";
                }
                return std::nullopt;
            }
            continue;
        }

        const QPoint result{m_shelf.x, m_shelf.y};
        m_shelf.x += width;
        m_shelf.height = std::max(m_shelf.height, height);
        return result;
    }
}

bool GlyphAtlas::growLocked() const
{
    const int oldSize = m_image.width();
    if (oldSize >= m_maxSize) {
        return false;
    }
    const int newSize = std::min(m_maxSize, oldSize * 2);

    QImage bigger(newSize, newSize, QImage::Format_RGBA8888);
    bigger.fill(Qt::transparent);
    for (int y = 0; y < m_image.height(); ++y) {
        std::memcpy(bigger.scanLine(y), m_image.constScanLine(y), static_cast<size_t>(oldSize) * 4);
    }
    m_image = std::move(bigger);

    // NOTE: glyph rects are texel coords, so they stay valid;
    // only the texture needs to be reallocated.
    m_resized = true;
    m_dirty = m_image.rect();
    return true;
}

void GlyphAtlas::markDirtyLocked(const QRect &rect) const
{
    m_dirty = m_dirty.isNull() ? rect : m_dirty.united(rect);
}

std::optional<AtlasUpload> GlyphAtlas::takePendingUpload() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_resized && m_dirty.isNull()) {
        return std::nullopt;
    }

    AtlasUpload upload;
    upload.resized = m_resized;
    upload.atlasWidth = m_image.width();
    upload.atlasHeight = m_image.height();
    upload.rect = m_resized ? m_image.rect() : m_dirty;
    // copy() is tightly packed (RGBA8 rows are 4-byte aligned)
    upload.image = m_image.copy(upload.rect);

    m_resized = false;
    m_dirty = QRect();
    return upload;
}

QSize GlyphAtlas::getSize() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_image.size();
}

bool GlyphAtlas::isFull() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_full;
}

QImage GlyphAtlas::copyImage() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_image.copy();
}

size_t GlyphAtlas::getNumGlyphs() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_glyphs.size();
}

} // namespace font
