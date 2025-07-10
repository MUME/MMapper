// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "gotowidget.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

GotoWidget::GotoWidget(QWidget *const parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText("Go to line");
    m_lineEdit->setValidator(new QIntValidator(1, 1000000, this));
    m_lineEdit->setMinimumWidth(80);
    layout->addWidget(m_lineEdit, 1);

    QToolButton *goButton = new QToolButton(this);
    goButton->setText("Go");
    goButton->setIcon(QIcon::fromTheme("go-jump", QIcon(":/icons/goto.png")));
    goButton->setToolTip("Go to specified line number");
    goButton->setAutoRaise(true);
    goButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    goButton->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(goButton);

    QToolButton *closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon::fromTheme("window-close", QIcon(":/icons/cancel.png")));
    closeButton->setToolTip("Close (Esc)");
    closeButton->setAutoRaise(true);
    closeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    closeButton->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(closeButton);

    setLayout(layout);

    auto gotoLine = [this]() {
        if (!m_lineEdit)
            return;
        bool ok;
        int lineNum = m_lineEdit->text().toInt(&ok);
        if (ok && lineNum > 0) {
            emit sig_gotoLineRequested(lineNum);
        } else {
            m_lineEdit->selectAll();
            m_lineEdit->setFocus();
        }
    };

    connect(m_lineEdit, &QLineEdit::returnPressed, this, gotoLine);
    connect(goButton, &QToolButton::clicked, this, gotoLine);
    connect(closeButton, &QToolButton::clicked, this, [this]() { emit sig_closeRequested(); });
}

GotoWidget::~GotoWidget() = default;

void GotoWidget::setFocusToInput()
{
    if (m_lineEdit) {
        m_lineEdit->setFocus(Qt::OtherFocusReason);
        m_lineEdit->clear();
    }
}

void GotoWidget::keyPressEvent(QKeyEvent *const event)
{
    if (event->key() == Qt::Key_Escape) {
        emit sig_closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}
