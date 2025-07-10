// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "findreplacewidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QSizePolicy>
#include <QToolButton>

FindReplaceWidget::FindReplaceWidget(bool allowReplace, QWidget *const parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);

    m_findLineEdit = new QLineEdit(this);
    m_findLineEdit->setPlaceholderText("Find");
    connect(m_findLineEdit,
            &QLineEdit::textChanged,
            this,
            &FindReplaceWidget::slot_updateButtonStates);
    layout->addWidget(m_findLineEdit, 0, 0);

    m_findPreviousButton = createActionButton("go-previous",
                                              ":/icons/layerup.png",
                                              "",
                                              "Find Previous",
                                              true,
                                              Qt::ToolButtonIconOnly);
    layout->addWidget(m_findPreviousButton, 0, 1, Qt::AlignVCenter);

    m_findNextButton = createActionButton("go-next",
                                          ":/icons/layerdown.png",
                                          "",
                                          "Find Next (Enter)",
                                          true,
                                          Qt::ToolButtonIconOnly);
    layout->addWidget(m_findNextButton, 0, 2, Qt::AlignVCenter);

    m_replaceToggleCheckBox = new QCheckBox("Replace", this);
    m_replaceToggleCheckBox->setToolTip("Show/Hide Replace Options");
    connect(m_replaceToggleCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_replaceLineEdit->setHidden(!checked);
        m_replaceCurrentButton->setHidden(!checked);
        m_replaceAllButton->setHidden(!checked);
        slot_updateButtonStates();
    });
    m_replaceToggleCheckBox->setEnabled(allowReplace);
    layout->addWidget(m_replaceToggleCheckBox, 0, 3, Qt::AlignVCenter | Qt::AlignRight);

    QToolButton *closeButton = createActionButton("window-close",
                                                  ":/icons/cancel.png",
                                                  "",
                                                  "Close (Esc)",
                                                  true,
                                                  Qt::ToolButtonIconOnly);
    layout->addWidget(closeButton, 0, 4, Qt::AlignVCenter | Qt::AlignRight);

    m_replaceLineEdit = new QLineEdit(this);
    m_replaceLineEdit->setPlaceholderText("Replace");
    layout->addWidget(m_replaceLineEdit, 1, 0);

    m_replaceCurrentButton = createActionButton("edit-find-replace",
                                                ":/icons/findreplace.png",
                                                "Replace",
                                                "Replace Current");
    layout->addWidget(m_replaceCurrentButton, 1, 1, 1, 2, Qt::AlignVCenter);

    m_replaceAllButton = createActionButton("dialog-ok-apply",
                                            ":/icons/apply.png",
                                            "All",
                                            "Replace All");
    layout->addWidget(m_replaceAllButton, 1, 3, 1, 2, Qt::AlignVCenter);

    layout->setColumnStretch(0, 10);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setColumnStretch(3, 0);
    layout->setColumnStretch(4, 0);

    setLayout(layout);

    m_replaceLineEdit->hide();
    m_replaceCurrentButton->hide();
    m_replaceAllButton->hide();

    auto findRequested = [this](QTextDocument::FindFlags flags) {
        if (!m_findLineEdit->text().isEmpty()) {
            emit sig_findRequested(m_findLineEdit->text(), flags);
        }
    };

    auto replaceCurrent = [this]() {
        if (!m_findLineEdit->text().isEmpty() && m_replaceToggleCheckBox->isChecked()) {
            emit sig_replaceCurrentRequested(m_findLineEdit->text(),
                                             m_replaceLineEdit->text(),
                                             QTextDocument::FindFlags());
        }
    };

    connect(m_findLineEdit, &QLineEdit::returnPressed, this, [findRequested]() {
        findRequested(QTextDocument::FindFlags());
    });
    connect(m_findNextButton, &QToolButton::clicked, this, [findRequested]() {
        findRequested(QTextDocument::FindFlags());
    });
    connect(m_findPreviousButton, &QToolButton::clicked, this, [findRequested]() {
        findRequested(QTextDocument::FindBackward);
    });
    connect(m_replaceLineEdit, &QLineEdit::returnPressed, this, replaceCurrent);
    connect(m_replaceCurrentButton, &QToolButton::clicked, this, replaceCurrent);
    connect(m_replaceAllButton, &QToolButton::clicked, this, [this]() {
        if (!m_findLineEdit->text().isEmpty() && m_replaceToggleCheckBox->isChecked()) {
            emit sig_replaceAllRequested(m_findLineEdit->text(),
                                         m_replaceLineEdit->text(),
                                         QTextDocument::FindFlags());
        }
    });
    connect(closeButton, &QToolButton::clicked, this, [this]() { emit sig_closeRequested(); });
}

FindReplaceWidget::~FindReplaceWidget() = default;

QToolButton *FindReplaceWidget::createActionButton(const QString &themeIcon,
                                                   const QString &qrcFallbackIcon,
                                                   const QString &text,
                                                   const QString &tooltip,
                                                   bool autoRaise,
                                                   Qt::ToolButtonStyle buttonStyle,
                                                   Qt::FocusPolicy focusPolicy)
{
    QToolButton *button = new QToolButton(this);
    button->setIcon(QIcon::fromTheme(themeIcon, QIcon(qrcFallbackIcon)));
    if (!text.isEmpty()) {
        button->setText(text);
    }
    button->setToolTip(tooltip);
    button->setAutoRaise(autoRaise);
    button->setToolButtonStyle(buttonStyle);
    button->setFocusPolicy(focusPolicy);
    return button;
}

void FindReplaceWidget::setFocusToFindInput()
{
    m_findLineEdit->setFocus(Qt::OtherFocusReason);
    m_findLineEdit->selectAll();
}

void FindReplaceWidget::slot_updateButtonStates()
{
    const bool findTextNotEmpty = !m_findLineEdit->text().isEmpty();
    m_findNextButton->setEnabled(findTextNotEmpty);
    m_findPreviousButton->setEnabled(findTextNotEmpty);

    const bool replaceChecked = m_replaceToggleCheckBox->isChecked();
    const bool enableReplaceButtons = findTextNotEmpty && replaceChecked;

    m_replaceCurrentButton->setEnabled(enableReplaceButtons);
    m_replaceAllButton->setEnabled(enableReplaceButtons);
}

void FindReplaceWidget::keyPressEvent(QKeyEvent *const event)
{
    if (event->key() == Qt::Key_Escape) {
        emit sig_closeRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}
