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

#include <QMessageLogContext>
#include <QString>
#include <QtWidgets>

#include "../expandoracommon/room.h"
#include "../global/Color.h"
#include "../global/roomid.h"
#include "../mapdata/mapdata.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "groupselection.h"
#include "mmapper2group.h"

class RoomSelection;

static constexpr const int GROUP_COLUMN_COUNT = 8;
static_assert(GROUP_COLUMN_COUNT == static_cast<int>(GroupModel::ColumnType::ROOM_NAME) + 1,
              "# of columns");

GroupModel::GroupModel(MapData *const md, Mmapper2Group *const group, QObject *const parent)
    : QAbstractTableModel(parent)
    , m_map(md)
    , m_group(group)
{}

void GroupModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

int GroupModel::rowCount(const QModelIndex & /* parent */) const
{
    if (auto group = m_group->getGroup()) {
        GroupSelection *selection = group->selectAll();
        int size = static_cast<int>(selection->size());
        group->unselect(selection);
        return size;
    }
    return 0;
}

int GroupModel::columnCount(const QModelIndex & /* parent */) const
{
    return GROUP_COLUMN_COUNT;
}

QString calculatePercentage(const int numerator, const int denomenator)
{
    if (denomenator == 0)
        return "";
    int percentage = static_cast<int>(100.0 * static_cast<double>(numerator)
                                      / static_cast<double>(denomenator));
    return QString("%1\%").arg(percentage);
}

QString calculateRatio(const int numerator, const int denomenator)
{
    if (numerator == 0 && denomenator == 0)
        return "";
    return QString("%1/%2").arg(numerator).arg(denomenator);
}

QVariant GroupModel::dataForCharacter(CGroupChar *const character, ColumnType column, int role) const
{
    // Map column to data
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case ColumnType::NAME:
            return character->getName();
        case ColumnType::HP_PERCENT:
            return calculatePercentage(character->hp, character->maxhp);
        case ColumnType::MANA_PERCENT:
            return calculatePercentage(character->mana, character->maxmana);
        case ColumnType::MOVES_PERCENT:
            return calculatePercentage(character->moves, character->maxmoves);
        case ColumnType::HP:
            return calculateRatio(character->hp, character->maxhp);
        case ColumnType::MANA:
            return calculateRatio(character->mana, character->maxmana);
        case ColumnType::MOVES:
            return calculateRatio(character->moves, character->maxmoves);
        case ColumnType::ROOM_NAME: {
            QString roomName = "Unknown";
            if (character->pos != DEFAULT_ROOMID) {
                const RoomSelection *roomSelection = m_map->select();
                if (const Room *r = m_map->getRoom(character->pos, roomSelection)) {
                    roomName = r->getName();
                }
                m_map->unselect(roomSelection);
            }
            return roomName;
        }
        default:
            qWarning() << "Unsupported column" << static_cast<int>(column);
        }
        break;
    case Qt::BackgroundRole:
        return character->getColor();
    case Qt::TextColorRole:
        return textColor(character->getColor());
    case Qt::TextAlignmentRole:
        if (column != ColumnType::NAME && column != ColumnType::ROOM_NAME) {
            return Qt::AlignCenter;
        }
    }

    return QVariant();
}

QVariant GroupModel::data(const QModelIndex &index, int role) const
{
    QVariant data = QVariant();
    if (!index.isValid())
        return data;

    if (auto group = m_group->getGroup()) {
        GroupSelection *selection = group->selectAll();

        // Map row to character
        if (index.row() < static_cast<int>(selection->size())) {
            CGroupChar *character = selection->at(index.row());
            ColumnType column = static_cast<ColumnType>(index.column());
            data = dataForCharacter(character, column, role);
        }

        group->unselect(selection);
    }
    return data;
}

QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Orientation::Horizontal) {
            switch (static_cast<ColumnType>(section)) {
            case ColumnType::NAME:
                return "Name";
            case ColumnType::HP_PERCENT:
                return "HP %";
            case ColumnType::MANA_PERCENT:
                return "Mana %";
            case ColumnType::MOVES_PERCENT:
                return "Moves %";
            case ColumnType::HP:
                return "HP";
            case ColumnType::MANA:
                return "Mana";
            case ColumnType::MOVES:
                return "Moves";
            case ColumnType::ROOM_NAME:
                return "Room Name";
            default:
                qWarning() << "Unsupported column" << section;
            }
        }
        break;
    }
    return QVariant();
}

Qt::ItemFlags GroupModel::flags(const QModelIndex & /* index */) const
{
    return Qt::ItemFlag::NoItemFlags;
}

GroupWidget::GroupWidget(Mmapper2Group *const group, MapData *const md, QWidget *const parent)
    : QWidget(parent)
    , m_group(group)
    , m_map(md)
    , m_model(md, group, this)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_table = new QTableView(this);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setModel(&m_model);
    layout->addWidget(m_table);

    hide();

    connect(m_group,
            &Mmapper2Group::drawCharacters,
            this,
            &GroupWidget::updateLabels,
            Qt::QueuedConnection);
    connect(m_group,
            &Mmapper2Group::messageBox,
            this,
            &GroupWidget::messageBox,
            Qt::QueuedConnection);
}

GroupWidget::~GroupWidget()
{
    delete m_table;
}

void GroupWidget::updateLabels()
{
    m_model.resetModel();
    m_table->resizeColumnsToContents();
}

void GroupWidget::messageBox(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}
