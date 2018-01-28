/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "groupwidget.h"
#include "mmapper2group.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "mapdata/mapdata.h"
#include "mmapper2room.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QList>
#include <QDebug>

#define GROUP_COLUMN_COUNT 8

GroupWidget::GroupWidget(Mmapper2Group *groupManager, MapData *md, QWidget *parent) :
    QTableWidget(1, GROUP_COLUMN_COUNT, parent),
    m_group(groupManager),
    m_map(md)
{
    setColumnCount(GROUP_COLUMN_COUNT);
    setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("HP %") << tr("Mana %") << tr("Moves %")
                              << tr("HP") << tr("Mana") << tr("Moves") << tr("Room Name"));
    verticalHeader()->setVisible(false);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::NoSelection);
    hide();

    connect(m_group, SIGNAL(drawCharacters()), SLOT(updateLabels()), Qt::QueuedConnection);
    connect(m_group, SIGNAL(messageBox(QString, QString)), SLOT(messageBox(QString, QString)),
            Qt::QueuedConnection);
}

GroupWidget::~GroupWidget()
{
    foreach (QString name, m_nameItemHash.keys()) {
        QList<QTableWidgetItem *> items = m_nameItemHash.take(name);
        for (uint i = 0; i < items.length(); i++) {
            delete items[i];
        }
    }
    m_nameItemHash.clear();
}

void GroupWidget::setItemText(QTableWidgetItem *item, const QString &text, const QColor &color)
{
    item->setText(text);
    item->setBackgroundColor(color);
    if (color.value() < 150)
        item->setTextColor(Qt::white);
    else
        item->setTextColor(Qt::black);
}

void GroupWidget::updateLabels()
{
    GroupSelection *selection = m_group->getGroup()->selectAll();

    // Clean up deleted characters
    QSet<QString> previous = QSet<QString>::fromList(m_nameItemHash.keys());
    QSet<QString> current = QSet<QString>::fromList(selection->keys());
    previous.subtract(current);
    if (!previous.empty()) {
        foreach (QString name, previous) {
            qDebug() << "Cleaning up item for character" << name;
            QList<QTableWidgetItem *> items = m_nameItemHash.take(name);
            if (!items.isEmpty()) {
                int row = items[0]->row();
                for (uint i = 0; i < items.length(); i++) {
                    QTableWidgetItem *item = takeItem(row, i);
                    delete item;
                }
            }
            qDebug() << "Done cleaning up item for character" << name;
        }
    }

    // Render the existing and new characters
    int row = 0;
    setRowCount(current.size());
    foreach (QString name, current) {
        CGroupChar *character = selection->value(name);
        QList<QTableWidgetItem *> items = m_nameItemHash[name];
        bool newItem = items.isEmpty();
        if (newItem) {
            qDebug() << "New item for character" << name << row;
            for (uint i = 0; i < GROUP_COLUMN_COUNT; i++) {
                QTableWidgetItem *item = new QTableWidgetItem("temp");
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                items.append(item);
            }
        }
        int previousRow = items[0]->row();
        bool moveItem = previousRow != row;
        if (newItem || moveItem) {
            for (uint i = 0; i < GROUP_COLUMN_COUNT; i++) {
                takeItem(previousRow, i);
            }
        }
        QTableWidgetItem *nameItem, *hpItemPct, *manaItemPct, *movesItemPct,
                         *hpItem, *manaItem, *movesItem, *roomNameItem;
        nameItem = items[0];
        hpItemPct = items[1];
        manaItemPct = items[2];
        movesItemPct = items[3];
        hpItem = items[4];
        manaItem = items[5];
        movesItem = items[6];
        roomNameItem = items[7];

        QColor color = character->getColor();
        setItemText(nameItem, character->name, color);
        double hpPercentage = 100.0 * (double)character->hp / (double)character->maxhp;
        double manaPercentage = 100.0 * (double)character->mana / (double)character->maxmana;
        double movesPercentage = 100.0 * (double)character->moves / (double)character->maxmoves;

        QString hpStrPct = qIsNaN(hpPercentage) ? "" : QString("%1\%").arg((int)hpPercentage);
        setItemText(hpItemPct, hpStrPct, color);
        QString manaStrPct = qIsNaN(manaPercentage) ? "" : QString("%1\%").arg((int)manaPercentage);
        setItemText(manaItemPct, manaStrPct, color);
        QString movesStrPct = qIsNaN(movesPercentage) ? "" : QString("%1\%").arg((int)movesPercentage);
        setItemText(movesItemPct, movesStrPct, color);

        QString hpStr = QString("%1/%2").arg(character->hp).arg(character->maxhp);
        setItemText(hpItem, hpStr, color);
        QString manaStr = QString("%1/%2").arg(character->mana).arg(character->maxmana);
        setItemText(manaItem, manaStr, color);
        QString movesStr = QString("%1/%2").arg(character->moves).arg(character->maxmoves);
        setItemText(movesItem, movesStr, color);

        QString roomName = "Unknown";
        if (character->pos != 0) {
            const RoomSelection *roomSelection = m_map->select();
            const Room *r = m_map->getRoom(character->pos, roomSelection);
            if (r) roomName = Mmapper2Room::getName(r);
            m_map->unselect(roomSelection);
        }
        setItemText(roomNameItem, roomName, color);

        if (newItem || moveItem) {
            setItem(row, 0, nameItem);
            setItem(row, 1, hpItemPct);
            setItem(row, 2, manaItemPct);
            setItem(row, 3, movesItemPct);
            setItem(row, 4, hpItem);
            setItem(row, 5, manaItem);
            setItem(row, 6, movesItem);
            setItem(row, 7, roomNameItem);
        }

        /*
        switch (state) {
          case BASHED:
            setItemText(item, 8, "BASHED", color);
            break;
          case INCAPACITATED:
            setItemText(item, 8, "INCAP", color);
            break;
          case DEAD:
            setItemText(item, 8, "DEAD", color);
            break;
          default:
            setItemText(item, 8, "Normal", color);
            break;
        }
        */

        // Increment current row
        row++;
    }
    m_group->getGroup()->unselect(selection);

    for (int i = 0; i < GROUP_COLUMN_COUNT; i++)
        resizeColumnToContents(i);
    horizontalHeader()->setStretchLastSection(true);
}

void GroupWidget::messageBox(QString title, QString message)
{
    QMessageBox::critical(this, title, message);
}

