#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include <QString>
#include <QWidget>
#include <QtCore>
#include <QtWidgets>

#include "adventuretracker.h"

class AdventureWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr const int MAX_LINES = 1024;
    static constexpr const auto DEFAULT_MSG
        = "Your adventures in Middle Earth will be tracked here! *BETA*\n";

    static constexpr const auto ACCOMPLISH_MSG = "Task accomplished! (%1 xp)\n";
    static constexpr const auto ACHIEVE_MSG = "Achievement: %1\n";
    static constexpr const auto ACHIEVE_MSG_XP = "Achievement: %1 (%2 xp)\n";
    static constexpr const auto DIED_MSG = "You are dead! Sorry... (%1 xp)\n";
    static constexpr const auto GAINED_LEVEL_MSG = "You gain a level! Congrats!\n";
    static constexpr const auto HINT_MSG = "Hint: %1\n";
    static constexpr const auto KILL_TROPHY_MSG = "Trophy: %1 (%2 xp)\n";

    explicit AdventureWidget(AdventureTracker &at, QWidget *parent = nullptr);

public slots:
    void slot_onAccomplishedTask(double xpGained);
    void slot_onAchievedSomething(const QString &achievement, double xpGained);
    void slot_onDied(double xpLost);
    void slot_onGainedLevel();
    void slot_onKilledMob(const QString &mobName, double xpGained);
    void slot_onReceivedHint(const QString &hint);

private slots:
    void slot_contextMenuRequested(const QPoint &pos);
    void slot_actionClearContent(bool checked = false);

private:
    void addDefaultContent();
    void addAdventureUpdate(const QString &msg);

    AdventureTracker &m_adventureTracker;

    QTextEdit *m_textEdit = nullptr;
    std::unique_ptr<QTextCursor> m_textCursor;
    QAction *m_clearContentAction = nullptr;
};
