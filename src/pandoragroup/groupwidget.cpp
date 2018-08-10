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

#include <QHash>
#include <QList>
#include <QMessageLogContext>
#include <QString>
#include <QtWidgets>

#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapdata/mapdata.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "groupselection.h"
#include "mmapper2group.h"

class RoomSelection;

static constexpr const int GROUP_COLUMN_COUNT = 8;

GroupWidget::GroupWidget(Mmapper2Group *groupManager, MapData *md, QWidget *parent)
    : QWidget(parent)
    , m_group(groupManager)
    , m_map(md)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_table = new QTableWidget(1, GROUP_COLUMN_COUNT, this);

    m_table->setColumnCount(GROUP_COLUMN_COUNT);
    m_table->setHorizontalHeaderLabels(QStringList()
                                       << tr("Name") << tr("HP %") << tr("Mana %") << tr("Moves %")
                                       << tr("HP") << tr("Mana") << tr("Moves") << tr("Room Name"));
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(m_table);

    hide();

    connect(m_group, &Mmapper2Group::drawCharacters, this, &GroupWidget::updateLabels, Qt::QueuedConnection);
    connect(m_group,
            &Mmapper2Group::messageBox,
            this, &GroupWidget::messageBox,
            Qt::QueuedConnection);
}

GroupWidget::~GroupWidget()
{
    for (const QString &name : m_nameItemHash.keys()) {
        QList<QTableWidgetItem *> items = m_nameItemHash.take(name);
        for (auto &item : items) {
            delete item;
        }
    }
    m_nameItemHash.clear();
    delete m_table;
}

void GroupWidget::setItemText(QTableWidgetItem *item, const QString &text, const QColor &color)
{
    item->setText(text);
    item->setBackgroundColor(color);
    if (color.value() < 150) {
        item->setTextColor(Qt::white);
    } else {
        item->setTextColor(Qt::black);
    }
}

void GroupWidget::updateLabels()
{
    GroupSelection *selection = m_group->getGroup()->selectAll();

    // Clean up deleted characters
    QSet<QString> previous = QSet<QString>::fromList(m_nameItemHash.keys());
    QSet<QString> current = QSet<QString>::fromList(selection->keys());
    previous.subtract(current);
    if (!previous.empty()) {
        for (const QString &name : previous) {
            qDebug() << "Cleaning up item for character" << name;
            QList<QTableWidgetItem *> items = m_nameItemHash.take(name);
            if (!items.isEmpty()) {
                int row = items[0]->row();
                for (int i = 0; i < items.length(); i++) {
                    QTableWidgetItem *item = m_table->takeItem(row, i);
                    delete item;
                }
            }
            qDebug() << "Done cleaning up item for character" << name;
        }
    }

    // Render the existing and new characters
    int row = 0;
    m_table->setRowCount(selection->keys().size());
    for (const QString &name : selection->keys()) {
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
        m_nameItemHash[name] = items;
        int previousRow = items[0]->row();
        bool moveItem = previousRow != row;
        if (newItem || moveItem) {
            for (uint i = 0; i < GROUP_COLUMN_COUNT; i++) {
                m_table->takeItem(previousRow, i);
            }
        }
        QTableWidgetItem *nameItem, *hpItemPct, *manaItemPct, *movesItemPct, *hpItem, *manaItem,
            *movesItem, *roomNameItem;
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
        double hpPercentage = 100.0 * static_cast<double>(character->hp)
                              / static_cast<double>(character->maxhp);
        double manaPercentage = 100.0 * static_cast<double>(character->mana)
                                / static_cast<double>(character->maxmana);
        double movesPercentage = 100.0 * static_cast<double>(character->moves)
                                 / static_cast<double>(character->maxmoves);

        QString hpStrPct = qIsNaN(hpPercentage)
                               ? ""
                               : QString("%1\%").arg(static_cast<int>(hpPercentage));
        setItemText(hpItemPct, hpStrPct, color);
        QString manaStrPct = qIsNaN(manaPercentage)
                                 ? ""
                                 : QString("%1\%").arg(static_cast<int>(manaPercentage));
        setItemText(manaItemPct, manaStrPct, color);
        QString movesStrPct = qIsNaN(movesPercentage)
                                  ? ""
                                  : QString("%1\%").arg(static_cast<int>(movesPercentage));
        setItemText(movesItemPct, movesStrPct, color);

        QString hpStr = QString("%1/%2").arg(character->hp).arg(character->maxhp);
        setItemText(hpItem, hpStr, color);
        QString manaStr = QString("%1/%2").arg(character->mana).arg(character->maxmana);
        setItemText(manaItem, manaStr, color);
        QString movesStr = QString("%1/%2").arg(character->moves).arg(character->maxmoves);
        setItemText(movesItem, movesStr, color);

        QString roomName = "Unknown";
        if (character->pos != DEFAULT_ROOMID) {
            const RoomSelection *roomSelection = m_map->select();
            const Room *r = m_map->getRoom(character->pos, roomSelection);
            if (r != nullptr) {
                roomName = r->getName();
            }
            m_map->unselect(roomSelection);
        }
        setItemText(roomNameItem, roomName, color);

        if (newItem || moveItem) {
            m_table->setItem(row, 0, nameItem);
            m_table->setItem(row, 1, hpItemPct);
            m_table->setItem(row, 2, manaItemPct);
            m_table->setItem(row, 3, movesItemPct);
            m_table->setItem(row, 4, hpItem);
            m_table->setItem(row, 5, manaItem);
            m_table->setItem(row, 6, movesItem);
            m_table->setItem(row, 7, roomNameItem);
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

    for (int i = 0; i < GROUP_COLUMN_COUNT; i++) {
        m_table->resizeColumnToContents(i);
    }
    m_table->horizontalHeader()->setStretchLastSection(true);
}

void GroupWidget::messageBox(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}
