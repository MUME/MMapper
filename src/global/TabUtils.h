#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "macros.h"

#include <QString>

namespace mmqt {
NODISCARD extern int measureExpandedTabsOneLine(QStringView line, int starting_at);
NODISCARD extern int measureExpandedTabsOneLine(const QString &line, int starting_at);
} // namespace mmqt

NODISCARD static inline constexpr int tab_advance(int col)
{
    return 8 - (col % 8);
}

NODISCARD static inline constexpr int next_tab_stop(int col)
{
    return col + tab_advance(col);
}
