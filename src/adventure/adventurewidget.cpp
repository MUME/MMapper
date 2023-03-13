// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include <QtCore>
#include <QtWidgets>

#include "adventuresession.h"
#include "adventurewidget.h"
#include "configuration/configuration.h"

AdventureWidget::AdventureWidget(AdventureTracker &at, QWidget *parent)
    : QWidget{parent}
    , m_adventureTracker{at}
{
    m_journalTextEdit = new QTextEdit(this);
    m_journalTextCursor = new QTextCursor(m_journalTextEdit->document());

    m_journalTextEdit->setReadOnly(true);
    m_journalTextEdit->setOverwriteMode(true);
    m_journalTextEdit->setUndoRedoEnabled(false);
    m_journalTextEdit->setDocumentTitle("Adventure Journal Text");
    m_journalTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_journalTextEdit->setTabChangesFocus(false);

    const auto settings = getConfig().integratedClient;

    QTextFrameFormat frameFormat = m_journalTextEdit->document()->rootFrame()->frameFormat();
    frameFormat.setBackground(settings.backgroundColor);
    m_journalTextEdit->document()->rootFrame()->setFrameFormat(frameFormat);

    QTextCharFormat blockCharFormat = m_journalTextCursor->blockCharFormat();
    blockCharFormat.setForeground(settings.foregroundColor);
    auto font = new QFont();
    font->fromString(settings.font); // needed fromString() to extract PointSize
    blockCharFormat.setFont(*font);
    m_journalTextCursor->setBlockCharFormat(blockCharFormat);

    auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_journalTextEdit);

    m_clearJournalAction = new QAction("Clear Content");
    connect(m_clearJournalAction,
            &QAction::triggered,
            this,
            &AdventureWidget::slot_actionClearContent);

    m_journalTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_journalTextEdit,
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

    connect(&m_adventureTracker, &AdventureTracker::sig_died, this, &AdventureWidget::slot_onDied);

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

    addDefaultContent();
}

void AdventureWidget::slot_onAccomplishedTask(double xpGained)
{
    // Only record accomplishedTask if it actually has associated xp to avoid
    // spam, since sometimes it co-triggers with achievement and can be redundant.
    if (xpGained > 0.0) {
        auto msg = QString(ACCOMPLISH_MSG).arg(AdventureSession::formatPoints(xpGained));
        addJournalEntry(msg);
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

    addJournalEntry(msg);
}

void AdventureWidget::slot_onDied(double xpLost)
{
    // Ignore Died messages that don't have an accompanying XP loss (to avoid whois spam, etc.)
    if (xpLost < 0.0) {
        auto msg = QString(DIED_MSG).arg(AdventureSession::formatPoints(xpLost));
        addJournalEntry(msg);
    }
}

void AdventureWidget::slot_onGainedLevel()
{
    addJournalEntry(QString(GAINED_LEVEL_MSG));
}

void AdventureWidget::slot_onKilledMob(const QString &mobName, double xpGained)
{
    // When player gets XP from multiple kills on the same heartbeat, as
    // frequently happens with quake xp, then the first mob gets all the XP
    // attributed and the others are zero. No way to solve this given
    // current MUME "protocol".
    auto xpf = (xpGained > 0.0) ? AdventureSession::formatPoints(xpGained) : "?";
    auto msg = QString(KILL_TROPHY_MSG).arg(mobName, xpf);

    addJournalEntry(msg);
}

void AdventureWidget::slot_onReceivedHint(const QString &hint)
{
    auto msg = QString(HINT_MSG).arg(hint);

    addJournalEntry(msg);
}

void AdventureWidget::slot_contextMenuRequested(const QPoint &pos)
{
    QMenu *contextMenu = m_journalTextEdit->createStandardContextMenu();
    contextMenu->addSeparator();
    contextMenu->addAction(m_clearJournalAction);
    contextMenu->exec(m_journalTextEdit->mapToGlobal(pos));
    delete contextMenu;
}

void AdventureWidget::slot_actionClearContent([[maybe_unused]] bool checked)
{
    m_journalTextCursor->movePosition(QTextCursor::Start);
    m_journalTextCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, QTextCursor::End);
    m_journalTextCursor->removeSelectedText();
    addDefaultContent();
}

void AdventureWidget::addDefaultContent()
{
    addJournalEntry(DEFAULT_MSG);
}

void AdventureWidget::addJournalEntry(const QString &msg)
{
    m_journalTextCursor->movePosition(QTextCursor::End);
    m_journalTextCursor->insertText(msg);

    // If more than MAX_LINES, preserve by deleting from the start
    auto lines_over = m_journalTextEdit->document()->lineCount() - AdventureWidget::MAX_LINES;
    if (lines_over > 0) {
        m_journalTextCursor->movePosition(QTextCursor::Start);
        m_journalTextCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_over);
        m_journalTextCursor->removeSelectedText();
        m_journalTextCursor->movePosition(QTextCursor::End);
    }

    // force scroll to bottom upon new message
    auto scrollBar = m_journalTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
