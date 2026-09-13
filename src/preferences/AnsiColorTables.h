#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/AnsiTextUtils.h"
#include "../global/macros.h"

#include <vector>

#include <QColor>
#include <QString>

// Widget-free home for the ANSI 16-color swatch table and string <-> color
// conversions used by preferences/ansicombo.{h,cpp}'s AnsiCombo widget (see
// AnsiCombo::initColours()/colorFromString()), which delegates to this file
// instead of duplicating the logic.
namespace AnsiColorTables {

// One entry in the 17-row fg/bg swatch list: index 0 is always the
// "default" (no color) entry, followed by one row per XFOREACH_ANSI_COLOR_0_7
// color in low-then-high order (black, BLACK, red, RED, ...), matching
// AnsiCombo::initColours()'s UI ordering.
struct NODISCARD Entry final
{
    AnsiColor16 color;
    QString description; // e.g. "none", "red", "RED"
};

NODISCARD const std::vector<Entry> &colorTable();

// Mirrors AnsiCombo::AnsiColor (see ansicombo.h): the decoded fg/bg/style
// state of a single ANSI escape-sequence string.
struct NODISCARD ParsedColor final
{
    AnsiColor16 bg;
    AnsiColor16 fg;
    bool bold = false;
    bool italic = false;
    bool underline = false;

    NODISCARD AnsiColor16Enum getBg() const { return bg.color.value_or(AnsiColor16Enum::white); }
    NODISCARD AnsiColor16Enum getFg() const { return fg.color.value_or(AnsiColor16Enum::black); }
    NODISCARD QColor getBgColor() const { return mmqt::toQColor(getBg()); }
    NODISCARD QColor getFgColor() const { return mmqt::toQColor(getFg()); }
    NODISCARD QString getBgName() const { return mmqt::toQStringUtf8(bg.to_string_view()); }
    NODISCARD QString getFgName() const { return mmqt::toQStringUtf8(fg.to_string_view()); }
};

// \return the decoded fg/bg/style state of an ANSI escape-sequence string
// like "[1;32m", or a default-constructed ParsedColor if it isn't valid.
NODISCARD ParsedColor colorFromString(const QString &ansiString);

// Builds the ANSI escape-sequence string with the leading ESC stripped (e.g.
// "[1;32m", the same representation Configuration::ParserSettings::
// roomNameColor/roomDescColor store) for the given fg/bg/style combination,
// or an empty string for the default (no style at all) combination. Mirrors
// AnsiColorDialog::slot_generateNewAnsiColor()'s getColor lambda.
NODISCARD QString
generateAnsiString(AnsiColor16 fg, AnsiColor16 bg, bool bold, bool italic, bool underline);

} // namespace AnsiColorTables
