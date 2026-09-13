#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "RemoteEditDocumentOps.h"

#include <QColor>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

// Widget-free syntax highlighter for the remote editor: flags tabs, overflowing lines,
// trailing whitespace, ANSI escapes, HTML-style entities, and encoding errors.
// QSyntaxHighlighter is QtGui (not QtWidgets), so this class has no widget dependency.
class NODISCARD RemoteEditHighlighter final : public QSyntaxHighlighter
{
private:
    const int m_maxLength;

public:
    explicit RemoteEditHighlighter(QTextDocument *document,
                                   int maxLength = mmqt::REMOTE_EDIT_MAX_LENGTH);
    ~RemoteEditHighlighter() final;

protected:
    void highlightBlock(const QString &line) override
    {
        highlightTabs(line);
        highlightOverflow(line);
        highlightTrailingSpace(line);
        highlightAnsi(line);
        highlightEntities(line);
        highlightEncodingErrors(line);
    }

private:
    NODISCARD static QTextCharFormat getBackgroundFormat(Qt::GlobalColor color);
    NODISCARD static QTextCharFormat getBackgroundFormat(const QColor &color);

    void highlightTabs(const QString &line);
    void highlightOverflow(const QString &line);
    void highlightTrailingSpace(const QString &line);
    void highlightAnsi(const QString &line);
    void highlightEntities(const QString &line);
    void highlightEncodingErrors(const QString &line);
};
