#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RoomMob.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include <QAbstractTableModel>
#include <QString>
#include <QStyledItemDelegate>
#include <QWidget>
#include <QtCore>

class RoomMobs;
class QObject;
class RoomManager;

class RoomModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class NODISCARD ColumnTypeEnum : uint8_t {
        NAME = 0,
        LABEL,
        POSITION,
        EFFECTS,
        WEAPON,
        FIGHTING,
        MOUNT,
    };

private:
    const RoomMobs &m_room;
    std::unordered_map<RoomMob::Id, SharedRoomMob> m_mobsById;
    std::vector<SharedRoomMob> m_mobVector;
    bool m_debug = false;

public:
    RoomModel(QObject *parent, const RoomMobs &room);

    NODISCARD int rowCount(const QModelIndex &parent) const override;
    NODISCARD int columnCount(const QModelIndex &parent) const override;

    NODISCARD QVariant data(const QModelIndex &index, int role) const override;
    NODISCARD QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    NODISCARD Qt::ItemFlags flags(const QModelIndex &parent) const override;

    void update();

private:
    NODISCARD SharedRoomMob getMob(int row) const;
    NODISCARD RoomMob::Field getField(ColumnTypeEnum column) const;
    NODISCARD const QVariant &getMobField(int row, int column) const;
    NODISCARD bool isEnemy(int row, int column) const;
    NODISCARD bool isFightingYOU(int row, int column) const;

}; // class RoomModel

class RoomWidget final : public QWidget
{
    Q_OBJECT

private:
    RoomModel m_model;
    RoomManager &m_roomManager;

public:
    explicit RoomWidget(RoomManager &rm, QWidget *parent);
    ~RoomWidget() final;

public slots:
    void slot_update();

signals:

private:
    void readSettings();
    void writeSettings();
};
