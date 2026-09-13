// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "AdventureLogModel.h"

#include "../global/Consts.h"
#include "adventuresession.h"

AdventureLogModel::AdventureLogModel(AdventureTracker &tracker, QObject *const parent)
    : QAbstractListModel(parent)
{
    addDefaultContent();

    connect(&tracker,
            &AdventureTracker::sig_accomplishedTask,
            this,
            &AdventureLogModel::slot_onAccomplishedTask);

    connect(&tracker,
            &AdventureTracker::sig_achievedSomething,
            this,
            &AdventureLogModel::slot_onAchievedSomething);

    connect(&tracker, &AdventureTracker::sig_diedInGame, this, &AdventureLogModel::slot_onDied);

    connect(&tracker,
            &AdventureTracker::sig_gainedLevel,
            this,
            &AdventureLogModel::slot_onGainedLevel);

    connect(&tracker, &AdventureTracker::sig_killedMob, this, &AdventureLogModel::slot_onKilledMob);

    connect(&tracker,
            &AdventureTracker::sig_receivedHint,
            this,
            &AdventureLogModel::slot_onReceivedHint);
}

int AdventureLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_lines.size());
}

QVariant AdventureLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || static_cast<qsizetype>(index.row()) >= m_lines.size()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return m_lines.at(index.row());
    }

    return QVariant();
}

QHash<int, QByteArray> AdventureLogModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    return roles;
}

void AdventureLogModel::clear()
{
    if (!m_lines.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, static_cast<int>(m_lines.size()) - 1);
        m_lines.clear();
        endRemoveRows();
    }
    addDefaultContent();
}

void AdventureLogModel::addDefaultContent()
{
    addAdventureUpdate(DEFAULT_MSG);
}

void AdventureLogModel::addAdventureUpdate(const QString &msg)
{
    QString line = msg;
    while (line.endsWith(char_consts::C_NEWLINE)) {
        line.chop(1);
    }

    const int row = static_cast<int>(m_lines.size());
    beginInsertRows(QModelIndex(), row, row);
    m_lines.append(line);
    endInsertRows();

    // If more than MAX_LINES, preserve by deleting from the start
    const int over = static_cast<int>(m_lines.size()) - MAX_LINES;
    if (over > 0) {
        beginRemoveRows(QModelIndex(), 0, over - 1);
        m_lines.erase(m_lines.begin(), m_lines.begin() + over);
        endRemoveRows();
    }
}

void AdventureLogModel::slot_onAccomplishedTask(double xpGained)
{
    // Only record accomplishedTask if it actually has associated xp to avoid
    // spam, since sometimes it co-triggers with achievement and can be redundant.
    if (xpGained > 0.0) {
        auto msg = QString(ACCOMPLISH_MSG).arg(AdventureSession::formatPoints(xpGained));
        addAdventureUpdate(msg);
    }
}

void AdventureLogModel::slot_onAchievedSomething(const QString &achievement, double xpGained)
{
    QString msg;

    if (xpGained > 0.0) {
        msg = QString(ACHIEVE_MSG_XP).arg(achievement, AdventureSession::formatPoints(xpGained));
    } else {
        msg = QString(ACHIEVE_MSG).arg(achievement);
    }

    addAdventureUpdate(msg);
}

void AdventureLogModel::slot_onDied(double xpLost)
{
    // Ignore Died messages that don't have an accompanying XP loss (to avoid whois spam, etc.)
    if (xpLost < 0.0) {
        auto msg = QString(DIED_MSG).arg(AdventureSession::formatPoints(xpLost));
        addAdventureUpdate(msg);
    }
}

void AdventureLogModel::slot_onGainedLevel()
{
    addAdventureUpdate(QString(GAINED_LEVEL_MSG));
}

void AdventureLogModel::slot_onKilledMob(const QString &mobName, double xpGained)
{
    // When player gets XP from multiple kills on the same heartbeat, as
    // frequently happens with quake xp, then the first mob gets all the XP
    // attributed and the others are zero. No way to solve this given
    // current MUME "protocol".
    auto xpf = (xpGained > 0.0) ? AdventureSession::formatPoints(xpGained) : "?";
    auto msg = QString(KILL_TROPHY_MSG).arg(mobName, xpf);

    addAdventureUpdate(msg);
}

void AdventureLogModel::slot_onReceivedHint(const QString &hint)
{
    auto msg = QString(HINT_MSG).arg(hint);

    addAdventureUpdate(msg);
}
