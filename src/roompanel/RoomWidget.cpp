// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RoomWidget.h"

#include "../configuration/configuration.h"
#include "../mapdata/mapdata.h"
#include "RoomManager.h"

#include <QAction>
#include <QColor>
#include <QHeaderView>
#include <QString>
#include <QStyledItemDelegate>
#include <QtWidgets>

// ------------------------------- RoomWidget ----------------------------------
RoomWidget::RoomWidget(RoomManager &rm, QWidget *const parent)
    : QWidget{parent}
    , m_roomManager{rm}
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_table = new QTableView(this);
    m_model = new RoomModel(m_table, rm.getRoom());
    m_table->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->setModel(m_model);
    layout->addWidget(m_table);

    // Minimize row height
    m_table->verticalHeader()->setDefaultSectionSize(
        m_table->verticalHeader()->minimumSectionSize());

    connect(&m_roomManager, &RoomManager::sig_updateWidget, this, &RoomWidget::slot_update);

    readSettings();
}

RoomWidget::~RoomWidget()
{
    writeSettings();
}

void RoomWidget::slot_update()
{
    deref(m_model).update();
}

void RoomWidget::readSettings()
{
    restoreGeometry(getConfig().roomPanel.geometry);
}

void RoomWidget::writeSettings()
{
    setConfig().roomPanel.geometry = saveGeometry();
}
