#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RoomModel.h"

#include <QStyledItemDelegate>
#include <QWidget>
#include <QtCore>

class RoomManager;
class QObject;
class QTableView;

class NODISCARD_QOBJECT RoomWidget final : public QWidget
{
    Q_OBJECT

private:
    RoomModel *m_model = nullptr;
    QTableView *m_table = nullptr;
    RoomManager &m_roomManager;

public:
    explicit RoomWidget(RoomManager &rm, QWidget *parent);
    ~RoomWidget() final;

private:
    void readSettings();
    void writeSettings();

public slots:
    void slot_update();
};
