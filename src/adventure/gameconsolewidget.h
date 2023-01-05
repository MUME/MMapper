#pragma once

#include "adventure/adventurejournal.h"
#include <QtWidgets>
#include <QWidget>

class GameConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    static const constexpr int MAX_LINES = 512;
    static const constexpr auto DEFAULT_CONTENT = "Narrates and Tells will appear here...";

    explicit GameConsoleWidget(AdventureJournal &aj, QWidget *parent = nullptr);

public slots:
    void slot_onKilledMob(const QString &mobName);
    void slot_onReceivedNarrate(const QString &narr);
    void slot_onReceivedTell(const QString &tell);

private:
    void addConsoleMessage(const QString &msg);

    AdventureJournal &m_adventureJournal;

    QTextDocument *m_consoleTextDoc = nullptr;
    QTextCursor *m_consoleCursor = nullptr;
    QTextEdit *m_consoleTextEdit = nullptr;

    int m_numMessagesReceived = 0;
};
