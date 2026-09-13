// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "AnsiColorTables.h"

#include "../global/TextUtils.h"

#include <cassert>
#include <functional>

#include <QDebug>
#include <QRegularExpression>

namespace AnsiColorTables {

const std::vector<Entry> &colorTable()
{
    static const std::vector<Entry> table = [] {
        std::vector<Entry> result;
        result.reserve(17);

        const auto add = [&result](const AnsiColor16 color) {
            Entry entry;
            entry.color = color;
            entry.description = mmqt::toQStringUtf8(color.isDefault() ? "none"
                                                                      : color.to_string_view());
            result.push_back(std::move(entry));
        };

        add(AnsiColor16{});

#define X_ADD(n, lower, UPPER) \
    add(AnsiColor16{AnsiColor16Enum::lower}); \
    add(AnsiColor16{AnsiColor16Enum::UPPER});
        XFOREACH_ANSI_COLOR_0_7(X_ADD)
#undef X_ADD

        assert(result.size() == 17);
        return result;
    }();
    return table;
}

ParsedColor colorFromString(const QString &colString)
{
    // No need to proceed if the color is empty
    if (colString.isEmpty()) {
        return ParsedColor{};
    }

    static const QRegularExpression re(R"(^\[((?:\d+[;:])*\d+)m$)");
    if (!re.match(colString).hasMatch()) {
        qWarning() << "String did not contain valid ANSI: " << colString;
        return ParsedColor{};
    }

    const AnsiColorState state = std::invoke([&colString]() -> AnsiColorState {
        AnsiColorState result_state;
        QString tmpStr = colString;
        tmpStr.chop(1);
        tmpStr.remove(0, 1);
        for (const auto &s : tmpStr.split(";", Qt::SplitBehaviorFlags::SkipEmptyParts)) {
            const auto n = s.toInt();
            result_state.receive(n);
        }
        return result_state;
    });

    if (!state.hasCompleteState()) {
        qWarning() << "String did not contain valid ANSI: " << colString;
    }

    const RawAnsi raw = state.getRawAnsi();

    ParsedColor color;
    color.fg = toAnsiColor16(raw.fg);
    color.bg = toAnsiColor16(raw.bg);
    color.bold = raw.hasBold();
    color.italic = raw.hasItalic();
    color.underline = raw.hasUnderline();
    return color;
}

QString generateAnsiString(const AnsiColor16 fg,
                           const AnsiColor16 bg,
                           const bool bold,
                           const bool italic,
                           const bool underline)
{
    RawAnsi raw;

    raw.fg = AnsiColorVariant{fg};
    raw.bg = AnsiColorVariant{bg};

    if (bold) {
        raw.setBold();
    }
    if (italic) {
        raw.setItalic();
    }
    if (underline) {
        raw.setUnderline();
    }

    if (raw == RawAnsi{}) {
        return "";
    }

    const AnsiString s = ansi_string(ANSI_COLOR_SUPPORT_HI, raw);
    if (s.isEmpty()) {
        return "";
    }

    auto sv = s.getStdStringView();
    assert(sv.front() == char_consts::C_ESC);
    assert(sv.back() == 'm');
    sv.remove_prefix(1);
    return mmqt::toQStringUtf8(sv);
}

} // namespace AnsiColorTables
