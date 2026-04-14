#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/macros.h"

#include <vector>

#include <QAbstractTableModel>
#include <QStringList>
#include <QTimer>

class CTimers;
class TTimer;
class QMimeData;

class NODISCARD_QOBJECT TimerModel final : public QAbstractTableModel
{
    Q_OBJECT

private:
    CTimers &m_timers;
    std::vector<const TTimer *> m_allTimers;
    QTimer m_refreshTimer;

public:
    enum Column { ColName = 0, ColTime, ColCount };
    enum Role { ProgressRole = Qt::UserRole + 1 };

public:
    explicit TimerModel(CTimers &timers, QObject *parent = nullptr);

    NODISCARD int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    NODISCARD int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    NODISCARD QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    NODISCARD QVariant headerData(int section,
                                  Qt::Orientation orientation,
                                  int role = Qt::DisplayRole) const override;

    NODISCARD Qt::ItemFlags flags(const QModelIndex &index) const override;
    NODISCARD Qt::DropActions supportedDropActions() const override;
    NODISCARD QStringList mimeTypes() const override;
    NODISCARD QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override;

    NODISCARD const TTimer *timerAt(int row) const;

private slots:
    void updateTimerList();

private:
    void startRefreshTimer();
};
