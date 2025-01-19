#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/AnsiTextUtils.h"
#include "../global/macros.h"

#include <optional>

#include <QColor>
#include <QFont>
#include <QSize>
#include <QString>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QtCore>
#include <QtGui>

class QObject;
class QResizeEvent;
class QTextDocument;
class QWidget;

struct NODISCARD FontDefaults final
{
    QFont serverOutputFont;
    QColor defaultBg = Qt::black;
    QColor defaultFg = Qt::lightGray;
    std::optional<QColor> defaultUl;

    explicit FontDefaults();
    NODISCARD QColor getDefaultUl() const { return defaultUl.value_or(defaultFg); }
};

extern void setDefaultFormat(QTextCharFormat &format, const FontDefaults &defaults);

NODISCARD extern RawAnsi updateFormat(QTextCharFormat &format,
                                      const FontDefaults &defaults,
                                      const RawAnsi &before,
                                      RawAnsi updated);

struct NODISCARD AnsiTextHelper final
{
    QTextEdit &textEdit;
    QTextCursor cursor;
    QTextCharFormat format;
    const FontDefaults defaults;
    RawAnsi currentAnsi;
    bool backspace = false;

    explicit AnsiTextHelper(QTextEdit &input_textEdit, FontDefaults def)
        : textEdit{input_textEdit}
        , cursor{textEdit.document()->rootFrame()->firstCursorPosition()}
        , format{cursor.charFormat()}
        , defaults{std::move(def)}
    {}

    explicit AnsiTextHelper(QTextEdit &input_textEdit)
        : AnsiTextHelper{input_textEdit, FontDefaults{}}
    {}

    void init();
    void displayText(const QString &str);
    void limitScrollback(int lineLimit);
};

class NODISCARD_QOBJECT DisplayWidget final : public QTextEdit
{
    Q_OBJECT

private:
    using base = QTextEdit;

private:
    AnsiTextHelper m_ansiTextHelper;
    bool m_canCopy = false;

public:
    explicit DisplayWidget(QWidget *parent);
    ~DisplayWidget() final;

private:
    NODISCARD const QFont &getDefaultFont() const
    {
        return m_ansiTextHelper.defaults.serverOutputFont;
    }

public:
    NODISCARD bool canCopy() const { return m_canCopy; }
    NODISCARD QSize sizeHint() const override;

public slots:
    void slot_displayText(const QString &str);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void sig_showMessage(const QString &, int);
    void sig_windowSizeChanged(int, int);
};
