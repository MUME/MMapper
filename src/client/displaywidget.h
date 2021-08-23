#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QColor>
#include <QFont>
#include <QSize>
#include <QString>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QtCore>
#include <QtGui>

#include "../global/macros.h"

class QObject;
class QResizeEvent;
class QTextDocument;
class QWidget;

class DisplayWidget final : public QTextEdit
{
private:
    using base = QTextEdit;

private:
    Q_OBJECT

public:
    explicit DisplayWidget(QWidget *parent);
    ~DisplayWidget() final;

    NODISCARD bool canCopy() const { return m_canCopy; }
    NODISCARD QSize sizeHint() const override;

private:
    bool m_canCopy = false;

public slots:
    void slot_displayText(const QString &str);

protected:
    QTextCursor m_cursor;
    QTextCharFormat m_format;

    void resizeEvent(QResizeEvent *event) override;

private:
    QColor m_foregroundColor;
    QColor m_backgroundColor;
    QFont m_serverOutputFont;
    // TODO: Create state machine
    bool m_ansi256Foreground = false;
    bool m_ansi256Background = false;
    bool m_backspace = false;

    void setDefaultFormat(QTextCharFormat &format);
    void updateFormat(QTextCharFormat &format, int ansiCode);
    void updateFormatBoldColor(QTextCharFormat &format);

signals:
    void sig_showMessage(const QString &, int);
    void sig_windowSizeChanged(int, int);
};
