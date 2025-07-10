#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QTextDocument>
#include <QToolButton>
#include <QWidget>

class QKeyEvent;

class NODISCARD_QOBJECT FindReplaceWidget final : public QWidget
{
    Q_OBJECT

private:
    QLineEdit *m_findLineEdit;
    QToolButton *m_findPreviousButton;
    QToolButton *m_findNextButton;
    QCheckBox *m_replaceToggleCheckBox;
    QLineEdit *m_replaceLineEdit;
    QToolButton *m_replaceCurrentButton;
    QToolButton *m_replaceAllButton;

public:
    explicit FindReplaceWidget(bool allowReplace, QWidget *parent = nullptr);
    ~FindReplaceWidget() override;

    void setFocusToFindInput();

signals:
    void sig_findRequested(const QString &term, QTextDocument::FindFlags flags);
    void sig_replaceCurrentRequested(const QString &findTerm,
                                     const QString &replaceTerm,
                                     QTextDocument::FindFlags flags);
    void sig_replaceAllRequested(const QString &findTerm,
                                 const QString &replaceTerm,
                                 QTextDocument::FindFlags flags);
    void sig_closeRequested();
    void sig_statusMessage(const QString &message);

private slots:
    void slot_updateButtonStates();

private:
    NODISCARD QToolButton *createActionButton(
        const QString &themeIcon,
        const QString &qrcFallbackIcon,
        const QString &text,
        const QString &tooltip,
        bool autoRaise = true,
        Qt::ToolButtonStyle buttonStyle = Qt::ToolButtonTextBesideIcon,
        Qt::FocusPolicy focusPolicy = Qt::NoFocus);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};
