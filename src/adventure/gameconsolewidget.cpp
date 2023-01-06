#include <QtCore>
#include <QtWidgets>

#include "gameconsolewidget.h"

GameConsoleWidget::GameConsoleWidget(AdventureJournal &aj, QWidget *parent)
    : QWidget{parent}
    , m_adventureJournal{aj}
{
    m_consoleTextDoc = new QTextDocument();
    m_consoleCursor = new QTextCursor(m_consoleTextDoc);

    m_consoleTextEdit = new QTextEdit(this);
    m_consoleTextEdit->setDocument(m_consoleTextDoc);

    m_consoleTextEdit->setReadOnly(true);
    m_consoleTextEdit->setOverwriteMode(true);
    m_consoleTextEdit->setUndoRedoEnabled(false);
    m_consoleTextEdit->setDocumentTitle("Game Console Text");
    m_consoleTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_consoleTextEdit->setTabChangesFocus(false);

    addConsoleMessage(DEFAULT_CONTENT);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_consoleTextEdit);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_killedMob,
            this,
            &GameConsoleWidget::slot_onKilledMob);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedNarrate,
            this,
            &GameConsoleWidget::slot_onReceivedNarrate);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedTell,
            this,
            &GameConsoleWidget::slot_onReceivedTell);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_updatedXP,
            this,
            &GameConsoleWidget::slot_onUpdatedXP);
}

void GameConsoleWidget::slot_onKilledMob(const QString &mobName)
{
    addConsoleMessage("Killed: " + mobName);
}

void GameConsoleWidget::slot_onReceivedNarrate(const QString &narr)
{
    addConsoleMessage(narr);
}

void GameConsoleWidget::slot_onReceivedTell(const QString &tell)
{
    addConsoleMessage(tell);
}

void GameConsoleWidget::slot_onUpdatedXP(const double currentXP)
{
    addConsoleMessage("Char XP updated: " + QString::number(currentXP));
}

void GameConsoleWidget::addConsoleMessage(const QString &msg)
{
    // If first message, clear the placeholder text
    auto prepend = "\n";
    if (m_numMessagesReceived == 0) {
        m_consoleTextDoc->clear();
        prepend = "";
    }
    m_numMessagesReceived++;

    m_consoleCursor->movePosition(QTextCursor::End);
    m_consoleCursor->insertText(prepend + msg);

    // If more than MAX_LINES, preserve by deleting from the start
    auto lines_over = m_consoleTextDoc->lineCount() - GameConsoleWidget::MAX_LINES;
    if (lines_over > 0) {
        m_consoleCursor->movePosition(QTextCursor::Start);
        m_consoleCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_over);
        m_consoleCursor->removeSelectedText();
        m_consoleCursor->movePosition(QTextCursor::End);
    }

    auto scrollBar = m_consoleTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
