#include <QtCore>
#include <QtWidgets>

#include "gameconsolewidget.h"

GameConsoleWidget::GameConsoleWidget(AdventureJournal &aj, QWidget *parent)
    : QWidget{parent}
    , m_adventureJournal{aj}
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_commsTextDocument = new QTextDocument(GameConsoleWidget::DEFAULT_CONTENT);
    m_commsCursor = new QTextCursor(m_commsTextDocument);
    m_commsTextEdit = new QTextEdit(this);
    m_commsTextEdit->setDocument(m_commsTextDocument);

    layout->addWidget(m_commsTextEdit);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedNarrate,
            this,
            &GameConsoleWidget::slot_onNarrateReceived);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedTell,
            this,
            &GameConsoleWidget::slot_onTellReceived);
}

void GameConsoleWidget::slot_onNarrateReceived(const QString &data)
{
    processMessage(data);
}

void GameConsoleWidget::slot_onTellReceived(const QString &data)
{
    processMessage(data);
}

void GameConsoleWidget::processMessage(const QString &msg)
{
    // If first message, clear the placeholder text
    auto prepend = "\n";
    if (m_numMessagesReceived == 0) {
        m_commsTextDocument->clear();
        prepend = "";
    }
    m_numMessagesReceived++;

    m_commsCursor->movePosition(QTextCursor::End);
    m_commsCursor->insertText(prepend + msg);

    // If more than MAX_LINES, preserve by deleting from the start
    auto lines_over = m_commsTextDocument->lineCount() - GameConsoleWidget::MAX_LINES;
    if (lines_over > 0) {
        m_commsCursor->movePosition(QTextCursor::Start);
        m_commsCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_over);
        m_commsCursor->removeSelectedText();
        m_commsCursor->movePosition(QTextCursor::End);
    }

    auto scrollBar = m_commsTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
