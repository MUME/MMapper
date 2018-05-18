/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "stackedinputwidget.h"
#include "inputwidget.h"

#include <QApplication>
#include <QDebug>
#include <QLineEdit>

StackedInputWidget::StackedInputWidget(QWidget *parent)
    : QStackedWidget(parent)
{
    // Multiline Input Widget
    m_inputWidget = new InputWidget(this);
    addWidget(m_inputWidget);
    connect(m_inputWidget, SIGNAL(sendUserInput(QString)), SLOT(gotMultiLineInput(QString)));
    connect(m_inputWidget, SIGNAL(displayMessage(QString)), SLOT(relayMessage(QString)));
    connect(m_inputWidget, SIGNAL(showMessage(const QString &, int)), SLOT(relayMessage(const QString &,
                                                                                        int)));

    // Password Widget
    m_passwordWidget = new QLineEdit(this);
    m_passwordWidget->setMaxLength(255);
    m_passwordWidget->setEchoMode(QLineEdit::Password);
    addWidget(m_passwordWidget);
    connect(m_passwordWidget, SIGNAL(returnPressed()), SLOT(gotPasswordInput()));

    // Grab focus
    setCurrentWidget(m_inputWidget);
    setFocusProxy(m_inputWidget);
    m_localEcho = true;
}

StackedInputWidget::~StackedInputWidget()
{
    m_inputWidget->disconnect();
    m_passwordWidget->disconnect();
    m_inputWidget->deleteLater();
    m_passwordWidget->deleteLater();
}

bool StackedInputWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        // Send KeyPress events to the input widget from the display widget
        if (m_localEcho) {
            QCoreApplication::sendEvent(m_inputWidget, event);
            m_inputWidget->setFocus();
        } else {
            QCoreApplication::sendEvent(m_passwordWidget, event);
            m_passwordWidget->setFocus();
        }
        return true;

    }
    // Standard event processing
    return QObject::eventFilter(obj, event);

}

void StackedInputWidget::toggleEchoMode(bool localEcho)
{
    m_localEcho = localEcho;
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
    emit sendUserInput(input.toLatin1());
}

void StackedInputWidget::gotMultiLineInput(QString input)
{
    QString str = input.append("\n");

    emit displayMessage(str);
    emit sendUserInput(str.toLatin1());
}

void StackedInputWidget::relayMessage(const QString &message)
{
    emit displayMessage(message);
}

void StackedInputWidget::relayMessage(const QString &message, int timeout)
{
    emit showMessage(message, timeout);
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
