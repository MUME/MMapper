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

namespace { // anonymous
const QVariant empty;
constexpr const uint8_t ROOM_COLUMN_COUNT = 7;
static_assert(ROOM_COLUMN_COUNT == static_cast<int>(RoomModel::ColumnTypeEnum::MOUNT) + 1,
              "# of columns");
} // namespace

// ---------------------------- RoomModel --------------------------------------

RoomModel::RoomModel(QObject *const parent, const RoomMobs &room)
    : QAbstractTableModel{parent}
    , m_room{room}
{}

int RoomModel::rowCount(const QModelIndex & /* parent */) const
{
    return std::max(1, static_cast<int>(m_mobVector.size()));
}

int RoomModel::columnCount(const QModelIndex & /* parent */) const
{
    return ROOM_COLUMN_COUNT;
}

QVariant RoomModel::headerData(int column, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<ColumnTypeEnum>(column)) {
        case ColumnTypeEnum::NAME:
            return "Name";
        case ColumnTypeEnum::LABEL:
            return "Label";
        case ColumnTypeEnum::POSITION:
            return "Position";
        case ColumnTypeEnum::EFFECTS:
            return "Effects";
        case ColumnTypeEnum::WEAPON:
            return "Weapon";
        case ColumnTypeEnum::FIGHTING:
            return "Fighting";
        case ColumnTypeEnum::MOUNT:
            return "Mount";
        default:
            if (m_debug) {
                qWarning() << "Unsupported column" << column;
            }
            break;
        }
    }
    return QVariant();
}

Qt::ItemFlags RoomModel::flags(const QModelIndex &index) const
{
    return Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable
           | QAbstractTableModel::flags(index);
}

QVariant RoomModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const int row = index.row();
        const int column = index.column();

        switch (role) {
        case Qt::DisplayRole:
            if (isFightingYOU(row, column)) {
                // replace you -> YOU for better visibility
                return QStringLiteral("YOU");
            }
            return getMobField(row, column);
        case Qt::BackgroundRole:
            if (isEnemy(row, column)) {
                // highlight enemy players
                // REVISIT: Ideally, this could be configurable.
                return QColor(Qt::yellow);
            }
            break;
        case Qt::ForegroundRole:
            if (isFightingYOU(row, column)) {
                // highlight "YOU" in fighting column
                return QColor(Qt::red);
            } else if (isEnemy(row, column)) {
                // highlight enemy players
                return mmqt::textColor(Qt::yellow);
            }
            break;
        default:
            break;
        }
    }
    return empty;
}

SharedRoomMob RoomModel::getMob(const int row) const
{
    if (row >= 0 && static_cast<size_t>(row) < m_mobVector.size()) {
        return m_mobVector.at(static_cast<size_t>(row));
    }
    return nullptr;
}

RoomMob::Field RoomModel::getField(const ColumnTypeEnum column) const
{
    switch (column) {
    case ColumnTypeEnum::NAME:
        return RoomMob::Field::NAME;
    case ColumnTypeEnum::LABEL:
        return RoomMob::Field::LABELS;
    case ColumnTypeEnum::POSITION:
        return RoomMob::Field::POSITION;
    case ColumnTypeEnum::EFFECTS:
        return RoomMob::Field::FLAGS;
    case ColumnTypeEnum::WEAPON:
        return RoomMob::Field::WEAPON;
    case ColumnTypeEnum::FIGHTING:
        return RoomMob::Field::FIGHTING;
    case ColumnTypeEnum::MOUNT:
        return RoomMob::Field::MOUNT;
    }
    abort();
}

const QVariant &RoomModel::getMobField(const int row, const int column) const
{
    SharedRoomMob mob = getMob(row);
    if (!mob) {
        return empty;
    }

    const RoomMob::Field i = getField(static_cast<ColumnTypeEnum>(column));
    const QVariant &variant = mob->getField(i);

    // immediately return non-Id based fields
    if (static_cast<QMetaType::Type>(variant.type()) != QMetaType::UInt) {
        return variant;
    }

    // field contains the ID of another mob: try to resolve it
    const auto iter = m_mobsById.find(variant.toUInt());
    if (iter != m_mobsById.end()) {
        const SharedRoomMob mob2 = iter->second;
        if (mob2) {
            return mob2->getField(RoomMob::Field::NAME);
        }
    }
    if (m_debug) {
        qDebug() << "mob id" << variant << "not found";
    }
    // failed to resolve ID of another mob:
    // clear the ID before MUME reuses it
    mob->setField(i, empty);
    return empty;
}

bool RoomModel::isEnemy(const int row, const int column) const
{
    if (column < static_cast<int>(ColumnTypeEnum::LABEL)
        || column > static_cast<int>(ColumnTypeEnum::WEAPON)) {
        const auto &variant = getMobField(row, column);
        if (variant == empty || !variant.canConvert(QMetaType::QString)) {
            return false;
        }

        // *an Orc*
        // *a grim Man*
        // *Foobar the Elf*
        // etc.
        const QString &str = variant.toString();
        return !str.isEmpty() && str.at(0) == mmqt::QC_ASTERISK;
    }
    return false;
}

bool RoomModel::isFightingYOU(const int row, const int column) const
{
    if (column == static_cast<int>(ColumnTypeEnum::FIGHTING)) {
        const SharedRoomMob mob = getMob(row);
        return mob && mob->getField(RoomMob::Field::FIGHTING).toString() == QStringLiteral("you");
    }
    return false;
}

void RoomModel::update()
{
    beginResetModel();
    m_room.updateModel(m_mobsById, m_mobVector);
    endResetModel();
}

// ------------------------------- RoomWidget ----------------------------------
RoomWidget::RoomWidget(RoomManager &rm, QWidget *const parent)
    : QWidget{parent}
    , m_model{this, rm.getRoom()}
    , m_roomManager{rm}
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *table = new QTableView(this);
    table->setSelectionMode(QAbstractItemView::ContiguousSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->setModel(&m_model);
    layout->addWidget(table);

    // Minimize row height
    table->verticalHeader()->setDefaultSectionSize(table->verticalHeader()->minimumSectionSize());

    connect(&m_roomManager, &RoomManager::sig_updateWidget, this, &RoomWidget::slot_update);

    readSettings();
}

RoomWidget::~RoomWidget()
{
    writeSettings();
}

void RoomWidget::slot_update()
{
    m_model.update();
}

void RoomWidget::readSettings()
{
    restoreGeometry(getConfig().roomPanel.geometry);
}

void RoomWidget::writeSettings()
{
    setConfig().roomPanel.geometry = saveGeometry();
}
