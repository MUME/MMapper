#pragma once

#include "adventure/adventuretracker.h"
#include <QString>
#include <QWidget>
#include <QtCore>
#include <QtWidgets>

class AdventureWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr const int MAX_LINES = 1024;
    static constexpr const auto DEFAULT_MSG
        = "Your progress in Middle Earth will be tracked here! *BETA*";

    static constexpr const auto TROPHY_MSG = "Trophy: %1 (%2 xp)";
    static constexpr const auto ACHIEVE_MSG = "Achievement: %1 (%2 xp)";
    static constexpr const auto DIED_MSG = "You are dead! Sorry... (%1 xp)";
    static constexpr const auto GAINED_LEVEL_MSG = "You gain a level! Congrats!";
    static constexpr const auto LOST_LEVEL_MSG = "Ouch! You lose a level! (%1 xp)";
    static constexpr const auto HINT_MSG = "Hint: %1";

    explicit AdventureWidget(AdventureTracker &at, QWidget *parent = nullptr);

    static const QString formatXPGained(const double xpGained);

public slots:
    void slot_onAchievedSomething(const QString &achivement, const double xpGained);
    void slot_onDied(const double xpLost);
    void slot_onGainedLevel();
    void slot_onKilledMob(const QString &mobName, const double xpGained);
    void slot_onLostLevel(const double xpLost);
    void slot_onReceivedHint(const QString &hint);

private:
    void addJournalEntry(const QString &msg);

    AdventureTracker &m_adventureTracker;

    QTextEdit *m_journalTextEdit = nullptr;
    QTextCursor *m_journalTextCursor = nullptr;

    int m_numMessagesReceived = 0;
};
