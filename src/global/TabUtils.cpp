// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "TabUtils.h"

#include "LineUtils.h"

static_assert(next_tab_stop(0) == 8);
static_assert(next_tab_stop(1) == 8);
static_assert(next_tab_stop(7) == 8);
static_assert(next_tab_stop(8) == 16);
static_assert(next_tab_stop(9) == 16);
static_assert(next_tab_stop(15) == 16);

namespace mmqt {
int measureExpandedTabsOneLine(const QStringView line, const int startingColumn)
{
    int col = startingColumn;
    for (const auto &c : line) {
        assert(c != char_consts::C_NEWLINE);
        if (c == char_consts::C_TAB) {
            col += 8 - (col % 8);
        } else {
            col += 1;
        }
    }
    return col;
}

int measureExpandedTabsOneLine(const QString &line, const int startingColumn)
{
    return measureExpandedTabsOneLine(QStringView{line}, startingColumn);
}

} // namespace mmqt
