// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupwidget.h"

#include <QAction>
#include <QHeaderView>
#include <QMessageLogContext>
#include <QString>
#include <QStyledItemDelegate>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../expandoracommon/room.h"
#include "../global/AnsiColor.h"
#include "../global/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "enums.h"
#include "groupselection.h"
#include "mmapper2group.h"

static constexpr const int GROUP_COLUMN_COUNT = 9;
static_assert(GROUP_COLUMN_COUNT == static_cast<int>(GroupModel::ColumnTypeEnum::ROOM_NAME) + 1,
              "# of columns");

GroupStateData::GroupStateData(const QColor &color,
                               const CharacterPositionEnum position,
                               const CharacterAffects affects)
    : color(std::move(color))
    , position(position)
    , affects(affects)
    , imageCount(1)
{
    // Increment imageCount for each active affect
    for (const auto affect : ALL_CHARACTER_AFFECTS) {
        if (affects.contains(affect)) {
            imageCount++;
        }
    }
}

void GroupStateData::paint(QPainter *const painter, const QRect &rect)
{
    painter->fillRect(rect, color);

    // REVISIT: Create questionmark icon?
    if (position == CharacterPositionEnum::UNDEFINED)
        return;

    // REVISIT: Build images ahead of time

    const bool invert = textColor(color) == Qt::white;
    const int x1 = [this, &rect]() {
        const int rectWidth = rect.right() - rect.x();
        const int imagesWidth = rect.height() * imageCount;
        return rect.x() + (rectWidth - imagesWidth) / 2;
    }();

    int currentImage = 0;
    const auto drawOne = [painter, &rect, invert, x1, &currentImage](auto &&filename) -> void {
        const auto getImage = [invert](auto &filename) {
            QImage image{filename};
            if (invert)
                image.invertPixels();
            return image;
        };

        QRect pixRect{x1 + currentImage * rect.height(),
                      rect.y(),
                      rect.height(), // REVISIT: width()?
                      rect.height()};
        painter->drawImage(pixRect, getImage(filename));
        currentImage++;
    };

    drawOne(getIconFilename(position));

    for (const auto affect : ALL_CHARACTER_AFFECTS) {
        if (affects.contains(affect)) {
            drawOne(getIconFilename(affect));
        }
    }
}

QSize GroupStateData::sizeHint() const
{
    return QSize(32 * imageCount, 32);
}

GroupDelegate::GroupDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

GroupDelegate::~GroupDelegate() = default;

void GroupDelegate::paint(QPainter *const painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    if (index.data().canConvert<GroupStateData>()) {
        GroupStateData stateData = qvariant_cast<GroupStateData>(index.data());
        stateData.paint(painter, option.rect);

    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize GroupDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data().canConvert<GroupStateData>()) {
        GroupStateData stateData = qvariant_cast<GroupStateData>(index.data());
        return stateData.sizeHint();
    } else {
        return QStyledItemDelegate::sizeHint(option, index);
    }
}

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
        auto selection = group->selectAll();
        return static_cast<int>(selection->size());
    }
    return 0;
}

int GroupModel::columnCount(const QModelIndex & /* parent */) const
{
    return GROUP_COLUMN_COUNT;
}

static QString calculatePercentage(const int numerator, const int denomenator)
{
    if (denomenator == 0)
        return "";
    int percentage = static_cast<int>(100.0 * static_cast<double>(numerator)
                                      / static_cast<double>(denomenator));
    // QT documentation doesn't say it's legal to use "\\%" or "%%", so we'll just append.
    return QString("%1").arg(percentage).append("%");
}

static QString calculateRatio(const int numerator, const int denomenator)
{
    if (numerator == 0 && denomenator == 0)
        return "";
    return QString("%1/%2").arg(numerator).arg(denomenator);
}

static QString getPrettyName(const CharacterPositionEnum position)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterPositionEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (position) {
        X_FOREACH_CHARACTER_POSITION(X_CASE)
    }
    return QString::asprintf("(CharacterPositionEnum)%d", static_cast<int>(position));
#undef X_CASE
}
static QString getPrettyName(const CharacterAffectEnum affect)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterAffectEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (affect) {
        X_FOREACH_CHARACTER_AFFECT(X_CASE)
    }
    return QString::asprintf("(CharacterAffectEnum)%d", static_cast<int>(affect));
#undef X_CASE
}

QVariant GroupModel::dataForCharacter(const SharedGroupChar &character,
                                      const ColumnTypeEnum column,
                                      const int role) const
{
    // Map column to data
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case ColumnTypeEnum::NAME:
            return character->getName();
        case ColumnTypeEnum::HP_PERCENT:
            return calculatePercentage(character->hp, character->maxhp);
        case ColumnTypeEnum::MANA_PERCENT:
            return calculatePercentage(character->mana, character->maxmana);
        case ColumnTypeEnum::MOVES_PERCENT:
            return calculatePercentage(character->moves, character->maxmoves);
        case ColumnTypeEnum::HP:
            return calculateRatio(character->hp, character->maxhp);
        case ColumnTypeEnum::MANA:
            return calculateRatio(character->mana, character->maxmana);
        case ColumnTypeEnum::MOVES:
            return calculateRatio(character->moves, character->maxmoves);
        case ColumnTypeEnum::STATE:
            return QVariant::fromValue(
                GroupStateData(character->getColor(), character->position, character->affects));
        case ColumnTypeEnum::ROOM_NAME:
            if (character->roomId != DEFAULT_ROOMID && character->roomId != INVALID_ROOMID
                && !m_map->isEmpty() && m_mapLoaded && character->roomId <= m_map->getMaxId()) {
                auto roomSelection = RoomSelection(*m_map);
                if (const Room *const r = roomSelection.getRoom(character->roomId)) {
                    return r->getName().toQString();
                }
            }
            return "Unknown";

        default:
            qWarning() << "Unsupported column" << static_cast<int>(column);
            break;
        }
        break;

    case Qt::BackgroundRole:
        return character->getColor();

    case Qt::ForegroundRole:
        return textColor(character->getColor());

    case Qt::TextAlignmentRole:
        if (column != ColumnTypeEnum::NAME && column != ColumnTypeEnum::ROOM_NAME) {
            // NOTE: There's no QVariant(AlignmentFlag) constructor.
            return static_cast<int>(Qt::AlignCenter);
        }
        break;

    case Qt::ToolTipRole:
        switch (column) {
        case ColumnTypeEnum::HP_PERCENT:
            return calculateRatio(character->hp, character->maxhp);
        case ColumnTypeEnum::MANA_PERCENT:
            return calculateRatio(character->mana, character->maxmana);
        case ColumnTypeEnum::MOVES_PERCENT:
            return calculateRatio(character->moves, character->maxmoves);
        case ColumnTypeEnum::STATE: {
            QString prettyName = getPrettyName(character->position);
            for (const auto affect : ALL_CHARACTER_AFFECTS) {
                if (character->affects.contains(affect)) {
                    prettyName.append(", ").append(getPrettyName(affect));
                }
            }
            return prettyName;
        };
        case ColumnTypeEnum::NAME:
        case ColumnTypeEnum::HP:
        case ColumnTypeEnum::MANA:
        case ColumnTypeEnum::MOVES:
        case ColumnTypeEnum::ROOM_NAME:
            break;
        };
        break;

    default:
        break;
    }

    return QVariant();
}

QVariant GroupModel::data(const QModelIndex &index, int role) const
{
    QVariant data = QVariant();
    if (!index.isValid())
        return data;

    if (auto group = m_group->getGroup()) {
        auto selection = group->selectAll();

        // Map row to character
        if (index.row() < static_cast<int>(selection->size())) {
            const SharedGroupChar &character = selection->at(index.row());
            const auto column = static_cast<ColumnTypeEnum>(index.column());
            data = dataForCharacter(character, column, role);
        }
    }
    return data;
}

QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Orientation::Horizontal) {
            switch (static_cast<ColumnTypeEnum>(section)) {
            case ColumnTypeEnum::NAME:
                return "Name";
            case ColumnTypeEnum::HP_PERCENT:
                return "HP";
            case ColumnTypeEnum::MANA_PERCENT:
                return "Mana";
            case ColumnTypeEnum::MOVES_PERCENT:
                return "Moves";
            case ColumnTypeEnum::HP:
                return "HP";
            case ColumnTypeEnum::MANA:
                return "Mana";
            case ColumnTypeEnum::MOVES:
                return "Moves";
            case ColumnTypeEnum::STATE:
                return "State";
            case ColumnTypeEnum::ROOM_NAME:
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
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setModel(&m_model);
    m_table->setItemDelegate(new GroupDelegate(this));
    layout->addWidget(m_table);

    // Minimize row height
    m_table->verticalHeader()->setDefaultSectionSize(
        m_table->verticalHeader()->minimumSectionSize());

    m_kick = new QAction(QIcon(":/icons/offline.png"), tr("&Kick"), this);
    connect(m_kick, &QAction::triggered, this, [this]() {
        try {
            m_group->getGroupManagerApi().kickCharacter(selectedCharacter);
        } catch (const std::exception &ex) {
            messageBox("Group Manager", QString::fromLatin1(ex.what()));
        }
    });

    connect(m_table, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        if (!index.isValid()) {
            return;
        }

        // Identify target
        if (auto group = m_group->getGroup()) {
            auto selection = group->selectAll();
            // Map row to character
            if (index.row() < static_cast<int>(selection->size())) {
                const SharedGroupChar &character = selection->at(index.row());
                selectedCharacter = character->getName();

                // Center map on the clicked character
                if (character->roomId != DEFAULT_ROOMID && character->roomId != INVALID_ROOMID
                    && !m_map->isEmpty() && character->roomId <= m_map->getMaxId()) {
                    auto roomSelection = RoomSelection(*m_map);
                    if (const Room *const r = roomSelection.getRoom(character->roomId)) {
                        const Coordinate &c = r->getPosition();
                        const auto worldPos = c.to_vec2() + glm::vec2{0.5f, 0.5f};
                        emit sig_center(worldPos); // connects to MapWindow
                    }
                }
            }

            // Build Context menu
            auto &settings = getConfig().groupManager;
            const bool isServer = settings.state == GroupManagerStateEnum::Server;
            const bool selectedSelf = settings.charName == selectedCharacter;
            if (isServer && !selectedSelf) {
                // All context menu actions are only actionable by the server right now
                m_kick->setText(QString("&Kick %1").arg(selectedCharacter.constData()));
                QMenu contextMenu(tr("Context menu"), this);
                contextMenu.addAction(m_kick);
                contextMenu.exec(QCursor::pos());
            }
        }
    });

    connect(m_group,
            &Mmapper2Group::updateWidget,
            this,
            &GroupWidget::updateLabels,
            Qt::QueuedConnection);
    connect(m_group,
            &Mmapper2Group::messageBox,
            this,
            &GroupWidget::messageBox,
            Qt::QueuedConnection);

    readSettings();
}

GroupWidget::~GroupWidget()
{
    delete m_table;
    delete m_kick;
    writeSettings();
}

void GroupWidget::updateLabels()
{
    m_model.resetModel();

    // Hide unnecessary columns like mana if everyone is a zorc/troll
    const auto one_character_had_mana = [this]() -> bool {
        auto selection = m_group->getGroup()->selectAll();
        for (const auto &character : *selection) {
            if (character->mana > 0) {
                return true;
            }
        }
        return false;
    };
    const bool hide_mana = !one_character_had_mana();
    m_table->setColumnHidden(static_cast<int>(GroupModel::ColumnTypeEnum::MANA), hide_mana);
    m_table->setColumnHidden(static_cast<int>(GroupModel::ColumnTypeEnum::MANA_PERCENT), hide_mana);
}

void GroupWidget::messageBox(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}

void GroupWidget::readSettings()
{
    restoreGeometry(getConfig().groupManager.geometry);
}

void GroupWidget::writeSettings()
{
    setConfig().groupManager.geometry = saveGeometry();
}
