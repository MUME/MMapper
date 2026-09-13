#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QSize>

// Layout policy for small (phone-sized) windows, shared by whatever shell
// hosts the UI; the shell decides how to realize it (see
// MainWindow::setCompactLayout()).
namespace CompactLayout {

// A screen narrower or shorter than this (logical pixels) starts in the
// compact layout: a single menu, one panel at a time, no toolbars.
static constexpr const int MIN_WIDTH = 720;
static constexpr const int MIN_HEIGHT = 480;

// Share of the window height given to the client (terminal) panel while
// compact; the map gets what remains.
static constexpr const double CLIENT_HEIGHT_FRACTION = 0.55;

// Font scale for the panel tab strip while compact, so the tabs are large
// enough for a finger.
static constexpr const double TAB_FONT_SCALE = 1.4;

NODISCARD constexpr bool isCompact(const QSize size) noexcept
{
    return size.width() < MIN_WIDTH || size.height() < MIN_HEIGHT;
}

// The size the policy is applied to. A window can't usefully exceed its
// screen, but the expanded layout's minimum size can force it to (phones),
// and it is that minimum the decision is about to change, so the screen
// bounds the window size.
NODISCARD constexpr QSize probeSize(const QSize window, const QSize screen) noexcept
{
    return screen.isValid() ? window.boundedTo(screen) : window;
}

NODISCARD constexpr int clientHeight(const QSize size) noexcept
{
    return static_cast<int>(static_cast<double>(size.height()) * CLIENT_HEIGHT_FRACTION);
}

} // namespace CompactLayout
