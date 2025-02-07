// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "stackedinputwidget.h"

#include "../global/Color.h"
#include "../global/utils.h"
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
    auto &passwordWidget = getPasswordWidget();

    addWidget(&inputWidget);

    // Password Widget
    passwordWidget.setMaxLength(255);
    passwordWidget.setEchoMode(QLineEdit::Password);
    addWidget(&passwordWidget);

    // Grab focus
    setCurrentWidget(&inputWidget);
    setFocusProxy(&inputWidget);

    // Swallow shortcuts
    inputWidget.installEventFilter(this);
    passwordWidget.installEventFilter(this);
}

StackedInputWidget::~StackedInputWidget() = default;

StackedInputWidget::Pipeline::~Pipeline()
{
    objs.inputWidget.reset();
    objs.passwordWidget.reset();
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
    };

    auto &out = m_pipeline.outputs.inputOutputs;
    out = std::make_unique<LocalInputWidgetOutputs>(*this);
    m_pipeline.objs.inputWidget = std::make_unique<InputWidget>(this, deref(out));
}

void StackedInputWidget::initPassword()
{
    struct NODISCARD PasswordInput final : public QLineEdit
    {
        PaletteManager m_paletteManager;

        explicit PasswordInput(QWidget *parent)
            : QLineEdit(parent)
        {
            m_paletteManager.init(*this, QColor(Qt::yellow).lighter(), Qt::lightGray);
        }
        NODISCARD bool event(QEvent *const event) override
        {
            m_paletteManager.tryUpdateFromFocusEvent(*this, deref(event).type());
            return QLineEdit::event(event);
        }
    };
    m_pipeline.objs.passwordWidget = std::make_unique<PasswordInput>(this);

    QObject::connect(&getPasswordWidget(), &QLineEdit::returnPressed, this, [this]() {
        gotPasswordInput();
    });
}

void StackedInputWidget::setEchoMode(const EchoMode echoMode)
{
    auto select = [this](QWidget &widget) {
        setFocusProxy(&widget);
        setCurrentWidget(&widget);
    };

    // we always clear the password box;
    // should we also clear the input box, in case the user started typing a password?
    getPasswordWidget().clear();

    m_echoMode = echoMode;
    if (m_echoMode == EchoMode::Visible) {
        select(getInputWidget());
    } else {
        select(getPasswordWidget());
    }
}

void StackedInputWidget::gotPasswordInput()
{
    auto &pw = getPasswordWidget();
    pw.selectAll();
    QString input = pw.text() + "\n";
    pw.clear();
    getOutput().sendUserInput(input, EchoMode::Hidden);
}

void StackedInputWidget::gotMultiLineInput(const QString &input)
{
    QString str = QString(input).append("\n");
    getOutput().sendUserInput(str, EchoMode::Visible);

    // REVISIT: Make color configurable
    // TODO: use the ANSI ostream interface instead of manually embedding ansi codes.
    QString displayStr = QString("\033[0;33m").append(input).append("\033[0m\n");
    getOutput().displayMessage(displayStr);
}

InputWidget &StackedInputWidget::getInputWidget() // NOLINT (no, it shouldn't be const)
{
    return deref(m_pipeline.objs.inputWidget);
}

QLineEdit &StackedInputWidget::getPasswordWidget() // NOLINT (no, it shouldn't be const)
{
    return deref(m_pipeline.objs.passwordWidget);
}

void StackedInputWidget::slot_cut()
{
    if (m_echoMode == EchoMode::Visible) {
        getInputWidget().cut();
    } else {
        // REVISIT: should we allow cutting from a password box?
        getPasswordWidget().cut();
    }
}

void StackedInputWidget::slot_copy()
{
    if (m_echoMode == EchoMode::Visible) {
        getInputWidget().copy();
    } else {
        // REVISIT: should we allow copying from a password box?
        getPasswordWidget().copy();
    }
}

void StackedInputWidget::slot_paste()
{
    if (m_echoMode == EchoMode::Visible) {
        getInputWidget().paste();
    } else {
        getPasswordWidget().paste();
    }
}
