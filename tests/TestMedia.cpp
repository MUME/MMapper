// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TestMedia.h"

#include "../src/media/StackBlur.h"

#include <algorithm>

#include <QImage>
#include <QtTest/QtTest>

namespace { // anonymous

NODISCARD QImage makeGradient(const int w, const int h)
{
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int r = (x * 255) / std::max(1, w - 1);
            const int g = (y * 255) / std::max(1, h - 1);
            image.setPixel(x, y, qRgba(r, g, 128, 255));
        }
    }
    return image;
}

} // namespace

TestMedia::TestMedia() = default;

TestMedia::~TestMedia() = default;

void TestMedia::stackBlurPreservesSize()
{
    QImage image = makeGradient(32, 24);
    const QSize before = image.size();
    mm::stackBlur(image, 3);
    QCOMPARE(image.size(), before);
}

void TestMedia::stackBlurChangesGradient()
{
    const QImage original = makeGradient(32, 24);
    QImage blurred = original;
    mm::stackBlur(blurred, 3);

    QCOMPARE(blurred.size(), original.size());
    QVERIFY(blurred != original);

    // Determinism: blurring an identical copy again from scratch must
    // produce byte-identical output.
    QImage blurredAgain = original;
    mm::stackBlur(blurredAgain, 3);
    QCOMPARE(blurredAgain, blurred);
}

void TestMedia::stackBlurZeroRadiusIsNoop()
{
    const QImage original = makeGradient(16, 16);
    QImage image = original;
    mm::stackBlur(image, 0);
    QCOMPARE(image, original);
}

void TestMedia::stackBlurHandlesTinyImage()
{
    QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
    image.setPixel(0, 0, qRgba(10, 20, 30, 255));

    // Must not crash even though the requested radius is far larger than
    // the image; DescriptionImageProvider clamps radius against (w-1)/2 and
    // (h-1)/2 before calling this, but stackBlur() itself must stay safe.
    mm::stackBlur(image, 5);
    QCOMPARE(image.size(), QSize(1, 1));

    QImage empty;
    mm::stackBlur(empty, 5); // must not crash on a null image either
    QVERIFY(empty.isNull());
}

QTEST_MAIN(TestMedia)

#include "TestMedia.moc"
