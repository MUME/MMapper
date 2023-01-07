#include <QtCore>
#include <QtWidgets>

#include "configuration/configuration.h"
#include "gameconsolewidget.h"

GameConsoleWidget::GameConsoleWidget(AdventureJournal &aj, QWidget *parent)
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

    addConsoleMessage(DEFAULT_MSG);

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
    // TODO protect this with a mutex
    // BUG if multiple mobs killed before next XP update, this
    // only saves the last one and attributes all XP to that.
    m_freshKill = true;
    m_freshKillMob = mobName;
}

void GameConsoleWidget::slot_onReceivedNarrate(const QString &narr)
{
    // Drop narrates for now since they can get spammy.
    // Need to evaluate whether they are worth it.
    // addConsoleMessage(narr);
}

void GameConsoleWidget::slot_onReceivedTell(const QString &tell)
{
    addConsoleMessage(tell);
}

void GameConsoleWidget::slot_onUpdatedXP(const double currentXP)
{
    // qDebug() << "XP updated: " + QString::number(currentXP);

    if (!m_xpCheckpoint.has_value()) {
        // first value of the session
        addConsoleMessage("Initial XP checkpoint: " + QString::number(currentXP));
        m_xpCheckpoint = currentXP;
    }

    // TODO protect this with a mutex
    if (m_freshKill) {
        double xpGained = currentXP - m_xpCheckpoint.value();
        auto msg = QString(TROPHY_MESSAGE).arg(m_freshKillMob).arg(formatXPGained(xpGained));
        addConsoleMessage(msg);

        m_xpCheckpoint.emplace(currentXP);
        m_freshKill = false;
        m_freshKillMob.clear();
    }
}

const QString GameConsoleWidget::formatXPGained(const double xpGained)
{
    //qDebug() << "formatting xpGained: " << xpGained;

    if (xpGained < 1000) {
        return QString::number(xpGained);
    }

    if (xpGained < (10 * 1000)) {
        return QString::number(xpGained / 1000, 'f', 1) + "k";
    }

    return QString::number(xpGained / 1000, 'f', 0) + "k";
}

void GameConsoleWidget::addConsoleMessage(const QString &msg)
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
    auto lines_over = m_consoleTextEdit->document()->lineCount() - GameConsoleWidget::MAX_LINES;
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
