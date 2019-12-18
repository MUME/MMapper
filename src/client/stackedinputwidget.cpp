// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "stackedinputwidget.h"

#include <QLineEdit>
#include <QString>

#include "inputwidget.h"

class QWidget;

StackedInputWidget::StackedInputWidget(QWidget *const parent)
    : QStackedWidget(parent)
{
    // Multiline Input Widget
    m_inputWidget = new InputWidget(this);
    addWidget(m_inputWidget);
    connect(m_inputWidget,
            &InputWidget::sendUserInput,
            this,
            &StackedInputWidget::gotMultiLineInput);
    connect(m_inputWidget, &InputWidget::displayMessage, this, [this](QString message) {
        emit displayMessage(message);
    });
    connect(m_inputWidget, &InputWidget::showMessage, this, [this](QString message, int timeout) {
        emit showMessage(message, timeout);
    });

    // Password Widget
    m_passwordWidget = new QLineEdit(this);
    m_passwordWidget->setMaxLength(255);
    m_passwordWidget->setEchoMode(QLineEdit::Password);
    addWidget(m_passwordWidget);
    connect(m_passwordWidget,
            &QLineEdit::returnPressed,
            this,
            &StackedInputWidget::gotPasswordInput);

    // Grab focus
    setCurrentWidget(m_inputWidget);
    setFocusProxy(m_inputWidget);
    m_localEcho = true;

    // Swallow shortcuts
    m_inputWidget->installEventFilter(this);
    m_passwordWidget->installEventFilter(this);
}

StackedInputWidget::~StackedInputWidget()
{
    m_inputWidget->disconnect();
    m_passwordWidget->disconnect();
    m_inputWidget->deleteLater();
    m_passwordWidget->deleteLater();
}

bool StackedInputWidget::eventFilter(QObject *const obj, QEvent *const event)
{
    if (event->type() == QEvent::KeyPress) {
        if (auto *const keyEvent = dynamic_cast<QKeyEvent *>(event)) {
            if (keyEvent->matches(QKeySequence::Copy) || keyEvent->matches(QKeySequence::Cut)
                || keyEvent->matches(QKeySequence::Paste)) {
                // Send event to the parent
                event->ignore();
                return true;
            }
        }
    }
    // Standard event processing
    return QObject::eventFilter(obj, event);
}

void StackedInputWidget::toggleEchoMode(const bool localEcho)
{
    m_localEcho = localEcho;
    m_passwordWidget->clear();
    if (m_localEcho) {
        setFocusProxy(m_inputWidget);
        setCurrentWidget(m_inputWidget);
    } else {
        setFocusProxy(m_passwordWidget);
        setCurrentWidget(m_passwordWidget);
    }
}

void StackedInputWidget::gotPasswordInput()
{
    m_passwordWidget->selectAll();
    QString input = m_passwordWidget->text() + "\n";
    m_passwordWidget->clear();
    emit sendUserInput(input);
}

void StackedInputWidget::gotMultiLineInput(const QString &input)
{
    QString str = QString(input).append("\n");
    emit sendUserInput(str);
    // REVISIT: Make color configurable
    QString displayStr = QString("\033[0;33m").append(input).append("\033[0m\n");
    emit displayMessage(displayStr);
}

void StackedInputWidget::cut()
{
    if (m_localEcho) {
        m_inputWidget->cut();
    } else {
        m_passwordWidget->cut();
    }
}

void StackedInputWidget::copy()
{
    if (m_localEcho) {
        m_inputWidget->copy();
    } else {
        m_passwordWidget->copy();
    }
}

void StackedInputWidget::paste()
{
    if (m_localEcho) {
        m_inputWidget->paste();
    } else {
        m_passwordWidget->paste();
    }
}
