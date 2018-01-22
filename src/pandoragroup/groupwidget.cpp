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

#include <QMessageBox>
#include <QDebug>

GroupWidget::GroupWidget(Mmapper2Group *groupManager, MapData *md, QWidget *parent) :
    QTreeWidget(parent),
    m_group(groupManager),
    m_map(md)
{
    setColumnCount(8);
    setHeaderLabels(QStringList() << tr("Name") << tr("HP") << tr("Mana") << tr("Moves") << tr("HP") <<
                    tr("Mana") << tr("Moves") << tr("Room Name"));
    setRootIsDecorated(false);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::NoSelection);
    clear();
    hide();

    connect(m_group, SIGNAL(drawCharacters()), SLOT(updateLabels()), Qt::QueuedConnection);
    connect(m_group, SIGNAL(messageBox(QString, QString)), SLOT(messageBox(QString, QString)),
            Qt::QueuedConnection);
}

GroupWidget::~GroupWidget()
{
    foreach (QTreeWidgetItem *item, m_nameItemHash.values()) {
        delete item;
    }
    m_nameItemHash.clear();
}

void GroupWidget::setItemText(QTreeWidgetItem *item, uint itemNumber, const QString &text,
                              const QColor &color)
{
    item->setText(itemNumber, text);
    item->setBackgroundColor(itemNumber, color);
    if (color.value() < 150)
        item->setTextColor(itemNumber, Qt::white);
    else
        item->setTextColor(itemNumber, Qt::black);
}

void GroupWidget::updateLabels()
{
    QSet<QByteArray> seenCharacters = QSet<QByteArray>::fromList(m_nameItemHash.keys());

    GroupSelection *selection = m_group->getGroup()->selectAll();
    foreach (CGroupChar *character, selection->values()) {
        QByteArray name = character->getName();
        seenCharacters.remove(name);
        QTreeWidgetItem *item;
        if (m_nameItemHash.contains(name)) {
            item = m_nameItemHash[name];
        } else {
            item = new QTreeWidgetItem(this);
            m_nameItemHash[name] = item;
            qDebug() << "New item for character" << name;
        }

        QColor color = character->getColor();
        setItemText(item, 0, character->name, color);
        setItemText(item, 1, character->textHP, color);
        setItemText(item, 2, character->textMana, color);
        setItemText(item, 3, character->textMoves, color);

        setItemText(item, 4, QString("%1/%2").arg(character->hp).arg(character->maxhp), color);
        setItemText(item, 5, QString("%1/%2").arg(character->mana).arg(character->maxmana), color);
        setItemText(item, 6, QString("%1/%2").arg(character->moves).arg(character->maxmoves), color);

        QString roomName = "Unknown";
        if (character->pos != 0) {
            const RoomSelection *roomSelection = m_map->select();
            const Room *r = m_map->getRoom(character->pos, roomSelection);
            if (r) roomName = QString((*r)[0].toString());
            m_map->unselect(roomSelection);
        }
        setItemText(item, 7, roomName, color);

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

    }
    m_group->getGroup()->unselect(selection);

    if (!seenCharacters.empty()) {
        foreach (QByteArray name, seenCharacters) {
            delete m_nameItemHash[name];
            m_nameItemHash.remove(name);
            qDebug() << "Cleaning up item for character" << name;
        }
    }
}

void GroupWidget::messageBox(QString title, QString message)
{
    QMessageBox::critical(this, title, message);
}

