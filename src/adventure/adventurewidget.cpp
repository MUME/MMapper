#include <QtCore>
#include <QtWidgets>

#include "adventurewidget.h"
#include "configuration/configuration.h"

AdventureWidget::AdventureWidget(AdventureTracker &aj, QWidget *parent)
    : QWidget{parent}
    , m_adventureJournal{aj}
{
    m_consoleTextEdit = new QTextEdit(this);
    m_consoleTextCursor = new QTextCursor(m_consoleTextEdit->document());

    m_consoleTextEdit->setReadOnly(true);
    m_consoleTextEdit->setOverwriteMode(true);
    m_consoleTextEdit->setUndoRedoEnabled(false);
    m_consoleTextEdit->setDocumentTitle("Game Console Text");
    m_consoleTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_consoleTextEdit->setTabChangesFocus(false);

    const auto &settings = getConfig().integratedClient;

    QTextFrameFormat frameFormat = m_consoleTextEdit->document()->rootFrame()->frameFormat();
    frameFormat.setBackground(settings.backgroundColor);
    m_consoleTextEdit->document()->rootFrame()->setFrameFormat(frameFormat);

    QTextCharFormat blockCharFormat = m_consoleTextCursor->blockCharFormat();
    blockCharFormat.setForeground(settings.foregroundColor);
    auto font = new QFont();
    font->fromString(settings.font); // needed fromString() to extract PointSize
    blockCharFormat.setFont(*font);
    m_consoleTextCursor->setBlockCharFormat(blockCharFormat);

    addJournalMsg(DEFAULT_MSG);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_consoleTextEdit);

    connect(&m_adventureJournal,
            &AdventureTracker::sig_killedMob,
            this,
            &AdventureWidget::slot_onKilledMob);

    connect(&m_adventureJournal,
            &AdventureTracker::sig_receivedNarrate,
            this,
            &AdventureWidget::slot_onReceivedNarrate);

    connect(&m_adventureJournal,
            &AdventureTracker::sig_receivedTell,
            this,
            &AdventureWidget::slot_onReceivedTell);

    connect(&m_adventureJournal,
            &AdventureTracker::sig_updatedXP,
            this,
            &AdventureWidget::slot_onUpdatedXP);
}

void AdventureWidget::slot_onKilledMob(const QString &mobName)
{
    double xpGained = m_xpCurrent.value() - m_xpCheckpoint.value();
    auto msg = QString(TROPHY_MESSAGE).arg(mobName).arg(formatXPGained(xpGained));
    addJournalMsg(msg);

    m_xpCheckpoint.emplace(m_xpCurrent.value());
}

void AdventureWidget::slot_onReceivedNarrate(const QString &narr)
{
    addJournalMsg(narr);
}

void AdventureWidget::slot_onReceivedTell(const QString &tell)
{
    addJournalMsg(tell);
}

void AdventureWidget::slot_onUpdatedXP(const double currentXP)
{
    if (!m_xpCheckpoint.has_value()) {
        // first value of the session
        qDebug() << "Initial XP checkpoint: " + QString::number(currentXP, 'f', 0);
        m_xpCheckpoint = currentXP;
    }

    m_xpCurrent = currentXP;
}

const QString AdventureWidget::formatXPGained(const double xpGained)
{
    if (xpGained < 1000) {
        return QString::number(xpGained);
    }

    if (xpGained < (10 * 1000)) {
        return QString::number(xpGained / 1000, 'f', 1) + "k";
    }

    return QString::number(xpGained / 1000, 'f', 0) + "k";
}

void AdventureWidget::addJournalMsg(const QString &msg)
{
    // TODO maybe clear the default/placeholder text?
    auto prepend = "\n";
    if (m_numMessagesReceived == 0) {
        prepend = "";
    }

    m_consoleTextCursor->movePosition(QTextCursor::End);
    m_consoleTextCursor->insertText(prepend + msg);
    m_numMessagesReceived++;

    // If more than MAX_LINES, preserve by deleting from the start
    auto lines_over = m_consoleTextEdit->document()->lineCount() - AdventureWidget::MAX_LINES;
    if (lines_over > 0) {
        m_consoleTextCursor->movePosition(QTextCursor::Start);
        m_consoleTextCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_over);
        m_consoleTextCursor->removeSelectedText();
        m_consoleTextCursor->movePosition(QTextCursor::End);
    }

    // force scroll to bottom upon new message
    auto scrollBar = m_consoleTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
