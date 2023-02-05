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

    const auto &settings = getConfig().integratedClient;

    QTextFrameFormat frameFormat = m_journalTextEdit->document()->rootFrame()->frameFormat();
    frameFormat.setBackground(settings.backgroundColor);
    m_journalTextEdit->document()->rootFrame()->setFrameFormat(frameFormat);

    QTextCharFormat blockCharFormat = m_journalTextCursor->blockCharFormat();
    blockCharFormat.setForeground(settings.foregroundColor);
    auto font = new QFont();
    font->fromString(settings.font); // needed fromString() to extract PointSize
    blockCharFormat.setFont(*font);
    m_journalTextCursor->setBlockCharFormat(blockCharFormat);

    addJournalEntry(DEFAULT_MSG);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_journalTextEdit);

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
}

void AdventureWidget::slot_onAccomplishedTask(double xpGained)
{
    // Only record accomplishedTask if it actually has associated xp to avoid
    // spam, since sometimes it co-triggers with achievemnt and can be redundant.
    if (xpGained > 0.0) {
        auto msg = QString(ACCOMPLISH_MSG)
                   + QString(XP_SUFFIX).arg(AdventureSession::formatPoints(xpGained));
        addJournalEntry(msg);
    }
}

void AdventureWidget::slot_onAchievedSomething(const QString &achievement, double xpGained)
{
    auto msg = QString(ACHIEVE_MSG).arg(achievement);

    if (xpGained > 0.0)
        msg = msg + QString(XP_SUFFIX).arg(AdventureSession::formatPoints(xpGained));

    addJournalEntry(msg);
}

void AdventureWidget::slot_onDied(double xpLost)
{
    auto msg = QString(DIED_MSG);

    if (xpLost < 0.0)
        msg = msg + QString(XP_SUFFIX).arg(AdventureSession::formatPoints(xpLost));

    addJournalEntry(msg);
}

void AdventureWidget::slot_onGainedLevel()
{
    addJournalEntry(QString(GAINED_LEVEL_MSG));
}

void AdventureWidget::slot_onKilledMob(const QString &mobName, double xpGained)
{
    auto msg = QString(KILL_TROPHY_MSG).arg(mobName);

    if (xpGained > 0.0) {
        msg = msg + QString(XP_SUFFIX).arg(AdventureSession::formatPoints(xpGained));
    } else {
        // When player gets XP from multiple kills on the same heartbeat, like
        // frequently happens with quake xp, then the first mob gets all the XP
        // attributed and the others are zero. No way to solve this given
        // current "protocol" of how MUME exposes information.
        msg = msg + QString(XP_SUFFIX).arg("?");
    }

    addJournalEntry(msg);
}

void AdventureWidget::slot_onReceivedHint(const QString &hint)
{
    auto msg = QString(HINT_MSG).arg(hint);

    addJournalEntry(msg);
}

void AdventureWidget::addJournalEntry(const QString &msg)
{
    const auto prepend = (m_numMessagesReceived == 0) ? "" : "\n";

    m_journalTextCursor->movePosition(QTextCursor::End);
    m_journalTextCursor->insertText(prepend + msg);
    m_numMessagesReceived++;

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
