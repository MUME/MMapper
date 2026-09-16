// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../src/configuration/configuration.h"
#include "../src/font/GlyphAtlas.h"

#include <QtGui/QFontDatabase>
#include <QtTest/QtTest>

class TestFont final : public QObject
{
    Q_OBJECT

private:
    QString m_family;

    NODISCARD QFont makeFont(const int pixelSize) const
    {
        QFont f(m_family);
        f.setPixelSize(pixelSize);
        return f;
    }

private slots:
    void initTestCase()
    {
        setEnteredMain();
        const QString ttfPath = QFINDTESTDATA("../src/resources/fonts/Cantarell-Regular.ttf");
        QVERIFY(!ttfPath.isEmpty());
        const int fontId = QFontDatabase::addApplicationFont(ttfPath);
        QVERIFY(fontId != -1);
        m_family = QFontDatabase::applicationFontFamilies(fontId).at(0);
    }

    void testPointSizeConversion()
    {
        // 11pt at 96 dpi is the ~15px em that master's baked Cantarell18 atlas had.
        QCOMPARE(font::pointSizeToPhysicalPixels(11, 96.f, 1.f), 15);
        QCOMPARE(font::pointSizeToPhysicalPixels(11, 96.f, 2.f), 29);
        QCOMPARE(font::pointSizeToPhysicalPixels(12, 72.f, 1.f), 12);
        // garbage in -> sane defaults
        QCOMPARE(font::pointSizeToPhysicalPixels(11, 0.f, 0.f), 15);
        QVERIFY(font::pointSizeToPhysicalPixels(0, 96.f, 1.f) >= 1);
    }

    void testShapeBasics()
    {
        font::GlyphAtlas::Config config;
        config.font = makeFont(15);
        const font::GlyphAtlas atlas(config);

        QVERIFY(atlas.getLineHeight() > 0);
        QVERIFY(atlas.getAscent() > 0);
        QVERIFY(atlas.getAscent() <= atlas.getLineHeight());

        const auto shaped = atlas.shape("Black Hill");
        QCOMPARE(shaped.glyphs.size(), static_cast<size_t>(10));
        // The advance is what Qt's own layout says it is (same hinting).
        const QFontMetrics fm(atlas.getFont());
        QCOMPARE(shaped.advance, fm.horizontalAdvance(QStringLiteral("Black Hill")));
        // ...and in the ballpark of master's baked Cantarell18 atlas (54px),
        // to catch a gross scale regression.
        QVERIFY2(shaped.advance >= 45 && shaped.advance <= 75,
                 qPrintable(QString::number(shaped.advance)));

        // Pen positions are monotonic for LTR text.
        for (size_t i = 1; i < shaped.glyphs.size(); ++i) {
            QVERIFY(shaped.glyphs[i].x >= shaped.glyphs[i - 1].x);
        }

        // Space has no ink; letters do, and they carry padding.
        QVERIFY(shaped.glyphs[5].glyph.isEmpty);
        const auto &B = shaped.glyphs[0].glyph;
        QVERIFY(!B.isEmpty);
        QVERIFY(!B.isColor);
        QVERIFY(B.width > 2 * font::GlyphAtlas::PADDING);
        QVERIFY(B.height > 2 * font::GlyphAtlas::PADDING);
        // 'B' sits on the baseline: quad bottom is one padding texel below it.
        QCOMPARE(B.yoffset, -font::GlyphAtlas::PADDING);
        // and its top is roughly the cap height
        QVERIFY(B.yoffset + B.height >= 8);
        QVERIFY(B.yoffset + B.height <= 14);

        // Distinct glyphs are rasterized once: B,l,a,c,k,H,i (l twice).
        QCOMPARE(atlas.getNumGlyphs(), static_cast<size_t>(8)); // includes the space
        std::ignore = atlas.shape("Black Hill");
        QCOMPARE(atlas.getNumGlyphs(), static_cast<size_t>(8));

        // Empty and whitespace-only strings are harmless.
        QCOMPARE(atlas.shape("").advance, 0);
        QVERIFY(atlas.shape("").glyphs.empty());
        QVERIFY(atlas.shape(" ").advance > 0);
    }

    void testAtlasPixels()
    {
        font::GlyphAtlas::Config config;
        config.font = makeFont(15);
        const font::GlyphAtlas atlas(config);

        const auto shaped = atlas.shape("H");
        const auto &g = shaped.glyphs.at(0).glyph;
        QVERIFY(!g.isEmpty);

        const QImage img = atlas.copyImage();
        QCOMPARE(img.format(), QImage::Format_RGBA8888);
        QCOMPARE(img.width(), font::GlyphAtlas::MIN_SIZE);

        // The synthetic white block backs the background/underline quads.
        const auto &bg = atlas.getBackground();
        for (int y = bg.y; y < bg.y + bg.height; ++y) {
            for (int x = bg.x; x < bg.x + bg.width; ++x) {
                QCOMPARE(img.pixel(x, y), qRgba(255, 255, 255, 255));
            }
        }

        // Padding ring is fully transparent, interior has white ink with coverage in alpha.
        int opaque = 0;
        for (int y = g.y; y < g.y + g.height; ++y) {
            for (int x = g.x; x < g.x + g.width; ++x) {
                const QRgb px = img.pixel(x, y);
                const bool onRing = x < g.x + font::GlyphAtlas::PADDING
                                    || x >= g.x + g.width - font::GlyphAtlas::PADDING
                                    || y < g.y + font::GlyphAtlas::PADDING
                                    || y >= g.y + g.height - font::GlyphAtlas::PADDING;
                if (onRing) {
                    QCOMPARE(qAlpha(px), 0);
                } else if (qAlpha(px) != 0) {
                    QCOMPARE(qRed(px), 255);
                    QCOMPARE(qGreen(px), 255);
                    QCOMPARE(qBlue(px), 255);
                    if (qAlpha(px) == 255) {
                        ++opaque;
                    }
                }
            }
        }
        // A hinted 'H' at 15px has solid stems, not just gray smear.
        QVERIFY(opaque > 0);
    }

    void testPendingUploads()
    {
        font::GlyphAtlas::Config config;
        config.font = makeFont(15);
        const font::GlyphAtlas atlas(config);

        // The initial (empty) atlas needs a full upload.
        auto first = atlas.takePendingUpload();
        QVERIFY(first.has_value());
        QVERIFY(first->resized);
        QCOMPARE(first->rect, QRect(0, 0, first->atlasWidth, first->atlasHeight));
        QCOMPARE(first->image.size(), first->rect.size());
        QVERIFY(!atlas.takePendingUpload().has_value());

        // New glyphs only dirty the region they were packed into.
        const auto shaped = atlas.shape("xyz");
        auto second = atlas.takePendingUpload();
        QVERIFY(second.has_value());
        QVERIFY(!second->resized);
        QVERIFY(second->rect.width() < first->atlasWidth);
        for (const auto &sg : shaped.glyphs) {
            QVERIFY(second->rect.contains(
                QRect(sg.glyph.x, sg.glyph.y, sg.glyph.width, sg.glyph.height)));
        }
        QCOMPARE(second->image.size(), second->rect.size());
        QCOMPARE(second->image.format(), QImage::Format_RGBA8888);
        QVERIFY(!atlas.takePendingUpload().has_value());

        // Re-shaping cached text uploads nothing.
        std::ignore = atlas.shape("xyz");
        QVERIFY(!atlas.takePendingUpload().has_value());
    }

    void testGrowth()
    {
        font::GlyphAtlas::Config config;
        config.font = makeFont(120);
        config.initialSize = 256;
        config.maxSize = 1024;
        const font::GlyphAtlas atlas(config);
        std::ignore = atlas.takePendingUpload();

        const auto before = atlas.shape("AB");
        QCOMPARE(atlas.getSize(), QSize(256, 256));

        // Enough 120px glyphs to overflow 256^2 several times.
        const auto shaped = atlas.shape("CDEFGHIJKLMNOPQRSTUVWXYZ");
        QVERIFY(atlas.getSize().width() > 256);
        QVERIFY(atlas.getSize().width() <= 1024);
        QVERIFY(!atlas.isFull());

        auto upload = atlas.takePendingUpload();
        QVERIFY(upload.has_value());
        QVERIFY(upload->resized);
        QCOMPARE(upload->image.size(), atlas.getSize());

        // Glyph rects survive growth unchanged (texel coordinates).
        const auto after = atlas.shape("AB");
        QCOMPARE(after.glyphs.at(0).glyph.x, before.glyphs.at(0).glyph.x);
        QCOMPARE(after.glyphs.at(0).glyph.y, before.glyphs.at(0).glyph.y);
        for (const auto &sg : shaped.glyphs) {
            QVERIFY(!sg.glyph.isEmpty);
            QVERIFY(sg.glyph.x + sg.glyph.width <= atlas.getSize().width());
            QVERIFY(sg.glyph.y + sg.glyph.height <= atlas.getSize().height());
        }
    }

    void testFullAtlasDegradesGracefully()
    {
        font::GlyphAtlas::Config config;
        config.font = makeFont(200);
        config.initialSize = 256;
        config.maxSize = 256;
        const font::GlyphAtlas atlas(config);

        const auto shaped = atlas.shape("ABCDEFGHIJ");
        QVERIFY(atlas.isFull());
        QCOMPARE(atlas.getSize(), QSize(256, 256));
        // Layout still works; glyphs that didn't fit are simply not drawn.
        QVERIFY(shaped.advance > 0);
        QCOMPARE(shaped.glyphs.size(), static_cast<size_t>(10));
        bool anyEmpty = false;
        for (const auto &sg : shaped.glyphs) {
            anyEmpty |= sg.glyph.isEmpty;
        }
        QVERIFY(anyEmpty);
    }

    void testUnicodeAndFallback()
    {
        font::GlyphAtlas::Config config;
        config.font = makeFont(15);
        const font::GlyphAtlas atlas(config);

        // Latin-1 and general punctuation shape to one glyph each.
        QCOMPARE(atlas.shape("\xC3\xA9").glyphs.size(), static_cast<size_t>(1));     // é
        QCOMPARE(atlas.shape("\xE2\x80\x94").glyphs.size(), static_cast<size_t>(1)); // em dash

        // Emoji go through font fallback. Whether a color font exists depends
        // on the system, so only require that shaping doesn't blow up.
        const auto emoji = atlas.shape("\xE2\x9A\xA1\xEF\xB8\x8F"); // U+26A1 U+FE0F
        QVERIFY(!emoji.glyphs.empty());
        QVERIFY(emoji.advance > 0);
    }
};

QTEST_MAIN(TestFont)
#include "TestFont.moc"
