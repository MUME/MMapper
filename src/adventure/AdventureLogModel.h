#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"
#include "adventuretracker.h"

#include <QAbstractListModel>
#include <QStringList>

class NODISCARD_QOBJECT AdventureLogModel final : public QAbstractListModel
{
    Q_OBJECT

private:
    QStringList m_lines;

public:
    static constexpr const int MAX_LINES = 1024;
    static constexpr const auto DEFAULT_MSG
        = "Your adventures in Middle Earth will be tracked here!\n";

    static constexpr const auto ACCOMPLISH_MSG = "Task accomplished! (%1 xp)\n";
    static constexpr const auto ACHIEVE_MSG = "Achievement: %1\n";
    static constexpr const auto ACHIEVE_MSG_XP = "Achievement: %1 (%2 xp)\n";
    static constexpr const auto DIED_MSG = "You are dead! Sorry... (%1 xp)\n";
    static constexpr const auto GAINED_LEVEL_MSG = "You gain a level! Congrats!\n";
    static constexpr const auto HINT_MSG = "Hint: %1\n";
    static constexpr const auto KILL_TROPHY_MSG = "Trophy: %1 (%2 xp)\n";

    explicit AdventureLogModel(AdventureTracker &tracker, QObject *parent = nullptr);

    NODISCARD int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    NODISCARD QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    NODISCARD QHash<int, QByteArray> roleNames() const override;

public slots:
    // Drops every line and re-adds the default welcome line.
    void clear();

private:
    void addDefaultContent();
    void addAdventureUpdate(const QString &msg);

private slots:
    void slot_onAccomplishedTask(double xpGained);
    void slot_onAchievedSomething(const QString &achievement, double xpGained);
    void slot_onDied(double xpLost);
    void slot_onGainedLevel();
    void slot_onKilledMob(const QString &mobName, double xpGained);
    void slot_onReceivedHint(const QString &hint);
};
