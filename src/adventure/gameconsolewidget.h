#pragma once

#include "adventure/adventurejournal.h"
#include <QtWidgets>
#include <QWidget>

class GameConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    static const constexpr int MAX_LINES = 1024;
    static const constexpr auto DEFAULT_CONTENT
        = "*BETA* This window will show player communication (tells)"
          " and XP gained from kills.";

    explicit GameConsoleWidget(AdventureJournal &aj, QWidget *parent = nullptr);

public slots:
    void slot_onKilledMob(const QString &mobName);
    void slot_onReceivedNarrate(const QString &narr);
    void slot_onReceivedTell(const QString &tell);
    void slot_onUpdatedXP(const double currentXP);

private:
    void addConsoleMessage(const QString &msg);

    AdventureJournal &m_adventureJournal;

    QTextEdit *m_consoleTextEdit = nullptr;
    QTextCursor *m_consoleTextCursor = nullptr;

    int m_numMessagesReceived = 0;

    std::optional<double> m_xpCheckpoint;
    bool m_freshKill = false;
    QString m_freshKillMobName;
};
