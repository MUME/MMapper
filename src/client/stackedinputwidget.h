#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "../global/utils.h"

#include <QStackedWidget>
#include <QString>
#include <QtCore>

class PasswordDialog;
class InputWidget;
class QEvent;
class QLineEdit;
class QObject;
class QPlainTextEdit;
class QWidget;

struct InputWidgetOutputs;

enum class NODISCARD EchoModeEnum : uint8_t { Hidden, Visible };

struct NODISCARD StackedInputWidgetOutputs
{
public:
    explicit StackedInputWidgetOutputs() = default;
    virtual ~StackedInputWidgetOutputs();
    DELETE_CTORS_AND_ASSIGN_OPS(StackedInputWidgetOutputs);

public:
    // sent to the mud
    void sendUserInput(const QString &msg) { virt_sendUserInput(msg); }
    // to be displayed by the built-in client
    void displayMessage(const QString &msg) { virt_displayMessage(msg); }
    // pop-up messages
    void showMessage(const QString &msg, const int timeout) { virt_showMessage(msg, timeout); }
    // request password
    void requestPassword() { virt_requestPassword(); }
    // scroll display (pageUp=true for PageUp, false for PageDown)
    void scrollDisplay(bool pageUp) { virt_scrollDisplay(pageUp); }

private:
    // sent to the mud
    virtual void virt_sendUserInput(const QString &msg) = 0;
    // to be displayed by the built-in client
    virtual void virt_displayMessage(const QString &msg) = 0;
    // pop-up messages
    virtual void virt_showMessage(const QString &msg, int timeout) = 0;
    // request password
    virtual void virt_requestPassword() = 0;
    // scroll display
    virtual void virt_scrollDisplay(bool pageUp) = 0;
};

class NODISCARD_QOBJECT StackedInputWidget final : public QStackedWidget
{
    Q_OBJECT

private:
    struct NODISCARD Pipeline final
    {
        struct NODISCARD Outputs final
        {
            std::unique_ptr<InputWidgetOutputs> inputOutputs;
        };

        struct NODISCARD Objects final
        {
            std::unique_ptr<InputWidget> inputWidget;
            std::unique_ptr<PasswordDialog> passwordDialog;
        };

        Outputs outputs;
        Objects objs;

        ~Pipeline();
    };
    Pipeline m_pipeline;
    EchoModeEnum m_echoMode = EchoModeEnum::Visible;
    StackedInputWidgetOutputs *m_output = nullptr;

public:
    explicit StackedInputWidget(QWidget *parent);
    ~StackedInputWidget() final;

private:
    void initPipeline();
    void initInput();
    void initPassword();

public:
    void init(StackedInputWidgetOutputs &output)
    {
        if (m_output != nullptr) {
            std::abort();
        }
        m_output = &output;
    }

private:
    // if this fails, it means you forgot to call init
    NODISCARD StackedInputWidgetOutputs &getOutput() { return deref(m_output); }

private:
    NODISCARD InputWidget &getInputWidget();
    NODISCARD PasswordDialog &getPasswordDialog();

public:
    void requestPassword();
    void setEchoMode(EchoModeEnum echoMode);
    EchoModeEnum getEchoMode() const { return m_echoMode; }

private:
    void gotMultiLineInput(const QString &);
    void gotPasswordInput(const QString &);
    void displayInputMessage(const QString &input);

public slots:
    void slot_cut();
    void slot_copy();
    void slot_paste();
};
