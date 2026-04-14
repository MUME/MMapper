// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "TimerModel.h"

#include "../global/TextUtils.h"
#include "CTimers.h"

#include <tuple>
#include <vector>

#include <QColor>
#include <QDateTime>
#include <QMimeData>

namespace {
QString formatMs(int64_t ms)
{
    bool negative = ms < 0;
    if (negative)
        ms = -ms;
    int64_t totalSecs = ms / 1000;
    int64_t h = totalSecs / 3600;
    int64_t m = (totalSecs / 60) % 60;
    int64_t s = totalSecs % 60;

    QString res;
    if (h > 0) {
        res += QString::number(h) + ":";
    }
    res += QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return negative ? "-" + res : res;
}
} // namespace

TimerModel::TimerModel(CTimers &timers, QObject *parent)
    : QAbstractTableModel(parent)
    , m_timers(timers)
{
    m_refreshTimer.setSingleShot(true);
    connect(&m_timers, &CTimers::sig_timerAdded, this, &TimerModel::updateTimerList);
    connect(&m_timers, &CTimers::sig_timerRemoved, this, &TimerModel::updateTimerList);
    connect(&m_timers, &CTimers::sig_timersUpdated, this, &TimerModel::updateTimerList);

    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_allTimers.empty())
            return;
        emit dataChanged(index(0, ColName),
                         index(static_cast<int>(m_allTimers.size()) - 1, ColCount - 1),
                         {Qt::DisplayRole, ProgressRole});
        startRefreshTimer();
    });

    updateTimerList();
}

int TimerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_allTimers.size());
}

int TimerModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColCount;
}

QVariant TimerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || static_cast<size_t>(index.row()) >= m_allTimers.size()) {
        return QVariant();
    }

    const TTimer *timer = m_allTimers[static_cast<size_t>(index.row())];

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        switch (index.column()) {
        case ColName: {
            QString name = mmqt::toQStringUtf8(timer->getName());
            if (!timer->getDescription().empty()) {
                name += " <" + mmqt::toQStringUtf8(timer->getDescription()) + ">";
            }
            return name;
        }
        case ColTime:
            if (timer->isCountdown()) {
                return formatMs(timer->remainingMs());
            } else {
                return formatMs(timer->elapsedMs());
            }
        }
    } else if (role == Qt::ForegroundRole) {
        if (timer->isExpired()) {
            return QColor(Qt::red);
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColTime) {
            return QVariant(static_cast<int>(Qt::AlignCenter));
        }
    } else if (role == ProgressRole) {
        if (timer->isCountdown() && timer->durationMs() > 0) {
            return static_cast<double>(timer->remainingMs())
                   / static_cast<double>(timer->durationMs());
        }
        return QVariant();
    }

    return QVariant();
}

QVariant TimerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColName:
            return tr("Name");
        case ColTime:
            return tr("Time");
        }
    }
    return QVariant();
}

Qt::ItemFlags TimerModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);
    if (index.isValid()) {
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    } else {
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
}

Qt::DropActions TimerModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList TimerModel::mimeTypes() const
{
    return {"application/x-mmapper-timer-index"};
}

QMimeData *TimerModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return nullptr;

    auto *data = new QMimeData();
    data->setData("application/x-mmapper-timer-index", QByteArray::number(indexes.at(0).row()));
    return data;
}

bool TimerModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    std::ignore = column;
    if (action != Qt::MoveAction)
        return false;

    if (!data->hasFormat("application/x-mmapper-timer-index"))
        return false;

    int from = data->data("application/x-mmapper-timer-index").toInt();
    int to = row;

    if (row == -1) {
        if (parent.isValid()) {
            to = parent.row();
        } else {
            to = rowCount();
        }
    }

    if (from == to || from == to - 1)
        return false;

    m_timers.moveTimer(from, to);
    return true;
}

const TTimer *TimerModel::timerAt(int row) const
{
    if (row < 0 || static_cast<size_t>(row) >= m_allTimers.size())
        return nullptr;
    return m_allTimers[static_cast<size_t>(row)];
}

void TimerModel::updateTimerList()
{
    beginResetModel();
    m_allTimers.clear();

    const auto &timers = m_timers.allTimers();
    std::vector<const TTimer *> active;
    std::vector<const TTimer *> expired;

    for (const auto &t : timers) {
        if (t.isExpired()) {
            expired.push_back(&t);
        } else {
            active.push_back(&t);
        }
    }

    m_allTimers.insert(m_allTimers.end(), active.begin(), active.end());
    m_allTimers.insert(m_allTimers.end(), expired.begin(), expired.end());

    endResetModel();

    if (m_allTimers.empty()) {
        m_refreshTimer.stop();
    } else if (!m_refreshTimer.isActive()) {
        startRefreshTimer();
    }
}

void TimerModel::startRefreshTimer()
{
    // 50ms provides smooth 20fps updates for the progress bar
    m_refreshTimer.start(50);
}
