// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "StackBlur.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <QImage>

namespace mm {

void stackBlur(QImage &image, const int radius)
{
    const int w = image.width();
    const int h = image.height();
    if (w <= 0 || h <= 0 || radius <= 0) {
        return;
    }

    const int div = 2 * radius + 1;
    std::vector<QRgb> linePixels;

    // Horizontal blur
    linePixels.resize(static_cast<size_t>(w));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            linePixels[static_cast<size_t>(x)] = image.pixel(x, y);
        }
        for (int x = 0; x < w; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            for (int i = -radius; i <= radius; ++i) {
                size_t idx = static_cast<size_t>(std::clamp(x + i, 0, w - 1));
                QRgb pixel = linePixels[idx];
                rSum += qRed(pixel);
                gSum += qGreen(pixel);
                bSum += qBlue(pixel);
                aSum += qAlpha(pixel);
            }
            image.setPixel(x, y, qRgba(rSum / div, gSum / div, bSum / div, aSum / div));
        }
    }

    // Vertical blur
    linePixels.resize(static_cast<size_t>(h));
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            linePixels[static_cast<size_t>(y)] = image.pixel(x, y);
        }
        for (int y = 0; y < h; ++y) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            for (int i = -radius; i <= radius; ++i) {
                size_t idy = static_cast<size_t>(std::clamp(y + i, 0, h - 1));
                QRgb pixel = linePixels[idy];
                rSum += qRed(pixel);
                gSum += qGreen(pixel);
                bSum += qBlue(pixel);
                aSum += qAlpha(pixel);
            }
            image.setPixel(x, y, qRgba(rSum / div, gSum / div, bSum / div, aSum / div));
        }
    }
}

} // namespace mm
