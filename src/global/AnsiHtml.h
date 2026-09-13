#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AnsiTextUtils.h"
#include "macros.h"

#include <optional>

#include <QColor>
#include <QString>
#include <QStringView>
#include <QTextCharFormat>
#include <QTextCursor>

class QTextDocument;

namespace mmqt {

// Widget-free equivalent of client/displaywidget.h's FontDefaults, containing only the
// colors needed to render ANSI text; no QFont, and no dependency on getConfig().
struct NODISCARD AnsiHtmlDefaults final
{
    QColor defaultBg = Qt::black;
    QColor defaultFg = Qt::lightGray;
    std::optional<QColor> defaultUl;

    NODISCARD QColor getDefaultUl() const { return defaultUl.value_or(defaultFg); }
};

void setAnsiDefaultFormat(QTextCharFormat &format, const AnsiHtmlDefaults &defaults);

NODISCARD RawAnsi updateAnsiFormat(QTextCharFormat &format,
                                   const AnsiHtmlDefaults &defaults,
                                   const RawAnsi &before,
                                   RawAnsi updated);

// Renders ANSI SGR text into a QTextDocument via a cursor positioned at the document's end.
// This is the widget-free core that client/displaywidget.h's AnsiTextHelper delegates to.
struct NODISCARD AnsiTextToDocument final
{
    QTextCursor cursor;
    QTextCharFormat format;
    AnsiHtmlDefaults defaults;
    RawAnsi currentAnsi;
    bool linkify = true;

    explicit AnsiTextToDocument(QTextDocument &document,
                                AnsiHtmlDefaults def,
                                bool input_linkify = true);

    void init();
    void displayText(QStringView str);
    void limitScrollback(int lineLimit);
};

} // namespace mmqt
