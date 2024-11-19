// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "adventurewidget.h"

#include "../configuration/configuration.h"
#include "adventuresession.h"

#include <memory>

#include <QtCore>
#include <QtWidgets>

AdventureWidget::AdventureWidget(AdventureTracker &at, QWidget *const parent)
    : QWidget{parent}
    , m_adventureTracker{at}
{
    m_textEdit = new QTextEdit(this);
    m_textCursor = std::make_unique<QTextCursor>(m_textEdit->document());

    m_textEdit->setReadOnly(true);
    m_textEdit->setOverwriteMode(true);
    m_textEdit->setUndoRedoEnabled(false);
    m_textEdit->setDocumentTitle("Adventure Panel Text");
    m_textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textEdit->setTabChangesFocus(false);

    const auto settings = getConfig().integratedClient;

    QTextFrameFormat frameFormat = m_textEdit->document()->rootFrame()->frameFormat();
    frameFormat.setBackground(settings.backgroundColor);
    m_textEdit->document()->rootFrame()->setFrameFormat(frameFormat);

    QTextCharFormat blockCharFormat = m_textCursor->blockCharFormat();
    blockCharFormat.setForeground(settings.foregroundColor);
    {
        QFont font;
        font.fromString(settings.font); // need fromString() to extract PointSize
        blockCharFormat.setFont(font);
    }
    m_textCursor->setBlockCharFormat(blockCharFormat);

    auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_textEdit);

    addDefaultContent();

    m_clearContentAction = new QAction("Clear Content", this);
    connect(m_clearContentAction,
            &QAction::triggered,
            this,
            &AdventureWidget::slot_actionClearContent);

    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit,
            &QTextEdit::customContextMenuRequested,
            this,
            &AdventureWidget::slot_contextMenuRequested);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_accomplishedTask,
            this,
            &AdventureWidget::slot_onAccomplishedTask);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_achievedSomething,
            this,
            &AdventureWidget::slot_onAchievedSomething);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_diedInGame,
            this,
            &AdventureWidget::slot_onDied);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_gainedLevel,
            this,
            &AdventureWidget::slot_onGainedLevel);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_killedMob,
            this,
            &AdventureWidget::slot_onKilledMob);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_receivedHint,
            this,
            &AdventureWidget::slot_onReceivedHint);
}

void AdventureWidget::slot_onAccomplishedTask(double xpGained)
{
    // Only record accomplishedTask if it actually has associated xp to avoid
    // spam, since sometimes it co-triggers with achievement and can be redundant.
    if (xpGained > 0.0) {
        auto msg = QString(ACCOMPLISH_MSG).arg(AdventureSession::formatPoints(xpGained));
        addAdventureUpdate(msg);
    }
}

void AdventureWidget::slot_onAchievedSomething(const QString &achievement, double xpGained)
{
    QString msg;

    if (xpGained > 0.0) {
        msg = QString(ACHIEVE_MSG_XP).arg(achievement, AdventureSession::formatPoints(xpGained));
    } else {
        msg = QString(ACHIEVE_MSG).arg(achievement);
    }

    addAdventureUpdate(msg);
}

void AdventureWidget::slot_onDied(double xpLost)
{
    // Ignore Died messages that don't have an accompanying XP loss (to avoid whois spam, etc.)
    if (xpLost < 0.0) {
        auto msg = QString(DIED_MSG).arg(AdventureSession::formatPoints(xpLost));
        addAdventureUpdate(msg);
    }
}

void AdventureWidget::slot_onGainedLevel()
{
    addAdventureUpdate(QString(GAINED_LEVEL_MSG));
}

void AdventureWidget::slot_onKilledMob(const QString &mobName, double xpGained)
{
    // When player gets XP from multiple kills on the same heartbeat, as
    // frequently happens with quake xp, then the first mob gets all the XP
    // attributed and the others are zero. No way to solve this given
    // current MUME "protocol".
    auto xpf = (xpGained > 0.0) ? AdventureSession::formatPoints(xpGained) : "?";
    auto msg = QString(KILL_TROPHY_MSG).arg(mobName, xpf);

    addAdventureUpdate(msg);
}

void AdventureWidget::slot_onReceivedHint(const QString &hint)
{
    auto msg = QString(HINT_MSG).arg(hint);

    addAdventureUpdate(msg);
}

void AdventureWidget::slot_contextMenuRequested(const QPoint &pos)
{
    QMenu *contextMenu = m_textEdit->createStandardContextMenu();
    contextMenu->addSeparator();
    contextMenu->addAction(m_clearContentAction);
    contextMenu->exec(m_textEdit->mapToGlobal(pos));
    delete contextMenu;
}

void AdventureWidget::slot_actionClearContent([[maybe_unused]] bool checked)
{
    // REVISIT should use m_textCursor->document()->clear() instead?
    m_textCursor->movePosition(QTextCursor::Start);
    m_textCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, QTextCursor::End);
    m_textCursor->removeSelectedText();
    addDefaultContent();
}

void AdventureWidget::addDefaultContent()
{
    addAdventureUpdate(DEFAULT_MSG);
}

void AdventureWidget::addAdventureUpdate(const QString &msg)
{
    m_textCursor->movePosition(QTextCursor::End);
    m_textCursor->insertText(msg);

    // If more than MAX_LINES, preserve by deleting from the start
    auto lines_over = m_textEdit->document()->lineCount() - AdventureWidget::MAX_LINES;
    if (lines_over > 0) {
        m_textCursor->movePosition(QTextCursor::Start);
        m_textCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_over);
        m_textCursor->removeSelectedText();
        m_textCursor->movePosition(QTextCursor::End);
    }

    // force scroll to bottom upon new message
    auto scrollBar = m_textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
