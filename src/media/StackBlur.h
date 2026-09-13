#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

class QImage;

namespace mm {

// In-place box-blur approximation (separable horizontal then vertical
// pass). `radius` must be >= 0; a radius of 0 leaves the image unchanged.
// `image` must already be in a format QImage::pixel()/setPixel() can
// address (e.g. Format_ARGB32_Premultiplied); 0x0 images are handled as a
// no-op.
extern void stackBlur(QImage &image, int radius);

} // namespace mm
