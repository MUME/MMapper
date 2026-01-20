// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "stackedinputwidget.h"

#include "../global/Color.h"
#include "../global/utils.h"
#include "PasswordDialog.h"
#include "inputwidget.h"

#include <QLineEdit>
#include <QString>

class QWidget;

StackedInputWidgetOutputs::~StackedInputWidgetOutputs() = default;

StackedInputWidget::StackedInputWidget(QWidget *const parent)
    : QStackedWidget(parent)
{
    // Multiline Input Widget

    initPipeline();

    auto &inputWidget = getInputWidget();

    addWidget(&inputWidget);

    // Grab focus
    setCurrentWidget(&inputWidget);
    setFocusProxy(&inputWidget);

    // Swallow shortcuts
    inputWidget.installEventFilter(this);
}

StackedInputWidget::~StackedInputWidget() = default;

StackedInputWidget::Pipeline::~Pipeline()
{
    objs.inputWidget.reset();
    objs.passwordDialog.reset();
}

void StackedInputWidget::initPipeline()
{
    initInput();
    initPassword();
}

void StackedInputWidget::initInput()
{
    class NODISCARD LocalInputWidgetOutputs final : public InputWidgetOutputs
    {
    private:
        StackedInputWidget &m_self;

    public:
        explicit LocalInputWidgetOutputs(StackedInputWidget &self)
            : m_self{self}
        {}

    private:
        NODISCARD StackedInputWidget &getSelf() { return m_self; }
        NODISCARD StackedInputWidgetOutputs &getOutput() { return getSelf().getOutput(); }

    private:
        void virt_sendUserInput(const QString &msg) final { getSelf().gotMultiLineInput(msg); }
        void virt_displayMessage(const QString &msg) final { getOutput().displayMessage(msg); }
        void virt_showMessage(const QString &msg, int timeout) final
        {
            getOutput().showMessage(msg, timeout);
        }

        void virt_gotPasswordInput(const QString &password) final
        {
            getSelf().gotPasswordInput(password);
        }

        void virt_scrollDisplay(bool pageUp) final { getOutput().scrollDisplay(pageUp); }

        std::optional<QString> virt_getHotkey(const Hotkey &hk) final
        {
            return getOutput().getHotkey(hk);
        }
    };

    auto &out = m_pipeline.outputs.inputOutputs;
    out = std::make_unique<LocalInputWidgetOutputs>(*this);
    m_pipeline.objs.inputWidget = std::make_unique<InputWidget>(this, deref(out));
}

void StackedInputWidget::initPassword()
{
    // Pass the outputs interface to the PasswordDialog constructor
    m_pipeline.objs.passwordDialog
        = std::make_unique<PasswordDialog>(deref(m_pipeline.outputs.inputOutputs), this);
}

void StackedInputWidget::setEchoMode(const EchoModeEnum echoMode)
{
    m_echoMode = echoMode;
    if (m_echoMode == EchoModeEnum::Visible) {
        getPasswordDialog().hide();

        auto widget = &getInputWidget();
        setFocusProxy(widget);
        setCurrentWidget(widget);
    } else {
        requestPassword();
    }
}

void StackedInputWidget::requestPassword()
{
    auto &dlg = getPasswordDialog();
    if (dlg.isActiveWindow() && dlg.isVisible()) {
        return;
    }

    const auto clampedGlobalPos = std::invoke([this, &dlg]() -> QPoint {
        auto &input = getInputWidget();
        QPoint cursorGlobalPos = input.mapToGlobal(input.cursorRect(input.textCursor()).topLeft());
        QPoint desiredGlobalPos = cursorGlobalPos - QPoint(dlg.width(), dlg.height());
        QPoint desiredLocalPos = input.mapFromGlobal(desiredGlobalPos);

        // Clamp within input widget
        QRect inputRect = input.rect();
        int x = std::max(inputRect.left(),
                         std::min(desiredLocalPos.x(), inputRect.right() - dlg.width()));
        int y = std::max(inputRect.top(),
                         std::min(desiredLocalPos.y(), inputRect.bottom() - dlg.height()));
        return input.mapToGlobal(QPoint(x, y));
    });
    dlg.move(clampedGlobalPos);
    dlg.show();
    dlg.raise();
    dlg.activateWindow();
}

void StackedInputWidget::gotMultiLineInput(const QString &input)
{
    getOutput().sendUserInput(input + "\n");
    displayInputMessage(input);
}

void StackedInputWidget::gotPasswordInput(const QString &input)
{
    getOutput().sendUserInput(input + "\n");
    displayInputMessage("******");
}

void StackedInputWidget::displayInputMessage(const QString &input)
{
    QString displayStr = QString("\033[0;33m").append(input).append("\033[0m\n");
    getOutput().displayMessage(displayStr);
}

InputWidget &StackedInputWidget::getInputWidget() // NOLINT (no, it shouldn't be const)
{
    return deref(m_pipeline.objs.inputWidget);
}

PasswordDialog &StackedInputWidget::getPasswordDialog() // NOLINT (no, it shouldn't be const)
{
    return deref(m_pipeline.objs.passwordDialog);
}

void StackedInputWidget::slot_cut()
{
    if (m_echoMode == EchoModeEnum::Visible) {
        getInputWidget().cut();
    }
}

void StackedInputWidget::slot_copy()
{
    if (m_echoMode == EchoModeEnum::Visible) {
        getInputWidget().copy();
    }
}

void StackedInputWidget::slot_paste()
{
    if (m_echoMode == EchoModeEnum::Visible) {
        getInputWidget().paste();
    }
}
