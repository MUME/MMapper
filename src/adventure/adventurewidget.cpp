#include <QtCore>
#include <QtWidgets>

#include "adventurewidget.h"
#include "configuration/configuration.h"

AdventureWidget::AdventureWidget(AdventureTracker &aj, QWidget *parent)
    : QWidget{parent}
    , m_adventureTracker{aj}
{
    m_journalTextEdit = new QTextEdit(this);
    m_journalTextCursor = new QTextCursor(m_journalTextEdit->document());

    m_journalTextEdit->setReadOnly(true);
    m_journalTextEdit->setOverwriteMode(true);
    m_journalTextEdit->setUndoRedoEnabled(false);
    m_journalTextEdit->setDocumentTitle("Game Console Text");
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
            &AdventureTracker::sig_killedMob,
            this,
            &AdventureWidget::slot_onKilledMob);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_receivedNarrate,
            this,
            &AdventureWidget::slot_onReceivedNarrate);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_receivedTell,
            this,
            &AdventureWidget::slot_onReceivedTell);

    connect(&m_adventureTracker,
            &AdventureTracker::sig_updatedXP,
            this,
            &AdventureWidget::slot_onUpdatedXP);
}

void AdventureWidget::slot_onKilledMob(const QString &mobName, const double xpGained)
{
    addJournalEntry(QString(TROPHY_MESSAGE).arg(mobName).arg(formatXPGained(xpGained)));
}

void AdventureWidget::slot_onReceivedNarrate(const QString &narr)
{
    addJournalEntry(narr);
}

void AdventureWidget::slot_onReceivedTell(const QString &tell)
{
    addJournalEntry(tell);
}

void AdventureWidget::slot_onUpdatedXP(const double currentXP) {}

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

void AdventureWidget::addJournalEntry(const QString &msg)
{
    // TODO maybe clear the default/placeholder text?
    auto prepend = "\n";
    if (m_numMessagesReceived == 0) {
        prepend = "";
    }

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
