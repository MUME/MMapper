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

    static constexpr const auto TROPHY_MESSAGE = "Trophy: %1 (%2 xp)";

    explicit AdventureWidget(AdventureTracker &aj, QWidget *parent = nullptr);

    static const QString formatXPGained(const double xpGained);

public slots:
    void slot_onKilledMob(const QString &mobName, const double xpGained);
    void slot_onReceivedNarrate(const QString &narr);
    void slot_onReceivedTell(const QString &tell);

private:
    void addJournalEntry(const QString &msg);

    AdventureTracker &m_adventureTracker;

    QTextEdit *m_journalTextEdit = nullptr;
    QTextCursor *m_journalTextCursor = nullptr;

    int m_numMessagesReceived = 0;
};
