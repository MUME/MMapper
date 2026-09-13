#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "Hotkey.h"
#include "InputHistory.h"
#include "PaletteManager.h"

#include <QEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore>

class QKeyEvent;
class QObject;
class QWidget;

struct NODISCARD InputWidgetOutputs
{
public:
    explicit InputWidgetOutputs() = default;
    virtual ~InputWidgetOutputs();
    DELETE_CTORS_AND_ASSIGN_OPS(InputWidgetOutputs);

public:
    void sendUserInput(const QString &msg) { virt_sendUserInput(msg); }
    void displayMessage(const QString &msg) { virt_displayMessage(msg); }
    void showMessage(const QString &msg, const int timeout) { virt_showMessage(msg, timeout); }
    void gotPasswordInput(const QString &password) { virt_gotPasswordInput(password); }
    void scrollDisplay(bool pageUp) { virt_scrollDisplay(pageUp); }
    std::optional<QString> getHotkey(const Hotkey &hk) { return virt_getHotkey(hk); }

private:
    virtual void virt_sendUserInput(const QString &msg) = 0;
    virtual void virt_displayMessage(const QString &msg) = 0;
    virtual void virt_showMessage(const QString &msg, int timeout) = 0;
    virtual void virt_gotPasswordInput(const QString &password) = 0;
    virtual void virt_scrollDisplay(bool pageUp) = 0;
    virtual std::optional<QString> virt_getHotkey(const Hotkey &hk) = 0;
};

class NODISCARD_QOBJECT InputWidget final : public QPlainTextEdit
{
    Q_OBJECT

private:
    using base = QPlainTextEdit;

private:
    InputWidgetOutputs &m_outputs;
    QString m_tabFragment;
    TabHistory m_tabHistory;
    InputHistory m_inputHistory;
    PaletteManager m_paletteManager;
    bool m_tabbing = false;
    bool m_handledInShortcutOverride = false;

public:
    explicit InputWidget(QWidget *parent, InputWidgetOutputs &);
    ~InputWidget() final;

    NODISCARD QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    void gotInput();
    NODISCARD bool tryHistory(int);
    NODISCARD bool handleCommandInput(Qt::Key key, Qt::KeyboardModifiers mods);
    NODISCARD bool handleTerminalShortcut(int key);
    NODISCARD bool handleBasicKey(int key);

public:
    // The Tab / Up / Down key actions; also driven by the touch input strip
    // (see ClientWidget::initTouchInputStrip()).
    void tabComplete();
    void forwardHistory();
    void backwardHistory();

private:
    void sendUserInput(const QString &msg) { m_outputs.sendUserInput(msg); }
    void sendCommandWithSeparator(const QString &command);
};
