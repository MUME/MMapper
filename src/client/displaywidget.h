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
#include <QTextBrowser>
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

extern void setAnsiText(QTextEdit *pEdit, std::string_view text);

struct DisplayWidgetOutputs
{
public:
    explicit DisplayWidgetOutputs() = default;
    virtual ~DisplayWidgetOutputs();
    DELETE_CTORS_AND_ASSIGN_OPS(DisplayWidgetOutputs);

public:
    void showMessage(const QString &msg, const int timeout) { virt_showMessage(msg, timeout); }
    void windowSizeChanged(const int width, const int height)
    {
        virt_windowSizeChanged(width, height);
    }

private:
    virtual void virt_showMessage(const QString &msg, int timeout) = 0;
    virtual void virt_windowSizeChanged(int width, int height) = 0;
};

class NODISCARD_QOBJECT DisplayWidget final : public QTextBrowser
{
    Q_OBJECT

private:
    using base = QTextBrowser;

private:
    DisplayWidgetOutputs *m_output = nullptr;
    AnsiTextHelper m_ansiTextHelper;
    bool m_canCopy = false;

public:
    explicit DisplayWidget(QWidget *parent);
    ~DisplayWidget() final;

public:
    void init(DisplayWidgetOutputs &output)
    {
        if (m_output != nullptr) {
            std::abort();
        }
        m_output = &output;
    }

private:
    // if this fails, it means you forgot to call init
    NODISCARD DisplayWidgetOutputs &getOutput() { return deref(m_output); }

private:
    NODISCARD const QFont &getDefaultFont() const
    {
        return m_ansiTextHelper.defaults.serverOutputFont;
    }

public:
    NODISCARD bool canCopy() const { return m_canCopy; }
    NODISCARD QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void slot_displayText(const QString &str);
};
