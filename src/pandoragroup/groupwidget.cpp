// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupwidget.h"

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../global/AnsiTextUtils.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "enums.h"
#include "groupselection.h"
#include "mmapper2group.h"

#include <map>

#include <QAction>
#include <QHeaderView>
#include <QMessageLogContext>
#include <QString>
#include <QStyledItemDelegate>
#include <QtWidgets>

static constexpr const int GROUP_COLUMN_COUNT = 9;
static_assert(GROUP_COLUMN_COUNT == static_cast<int>(GroupModel::ColumnTypeEnum::ROOM_NAME) + 1,
              "# of columns");

namespace { // anonymous

class NODISCARD GroupImageCache final
{
private:
    using Key = std::pair<QString, bool>;
    std::map<std::pair<QString, bool>, QImage> m_images;

public:
    explicit GroupImageCache() = default;
    ~GroupImageCache() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(GroupImageCache);

public:
    NODISCARD bool contains(const Key &key) const { return m_images.find(key) != m_images.end(); }
    NODISCARD QImage &getImage(const QString &filename, const bool invert)
    {
        const Key key{filename, invert};
        if (!contains(key)) {
            auto &&[it, added] = m_images.emplace(key, filename);
            if (!added) {
                std::abort();
            }
            QImage &image = it->second;
            if (invert) {
                image.invertPixels();
            }

            qInfo() << "created image" << filename << (invert ? "(inverted)" : "(regular)");
        }
        return m_images[key];
    }
};

NODISCARD static auto &getImage(const QString &filename, const bool invert)
{
    static GroupImageCache g_groupImageCache;

    // NOTE: Assumes we're single-threaded; otherwise we'd need a lock here.
    return g_groupImageCache.getImage(filename, invert);
}
} // namespace

GroupStateData::GroupStateData(const QColor &color,
                               const CharacterPositionEnum position,
                               const CharacterAffectFlags affects)
    : m_color(std::move(color))
    , m_position(position)
    , m_affects(affects)
{
    if (position != CharacterPositionEnum::UNDEFINED) {
        ++m_count;
    }
    // Increment imageCount for each active affect
    for (const CharacterAffectEnum affect : ALL_CHARACTER_AFFECTS) {
        if (affects.contains(affect)) {
            ++m_count;
        }
    }
    // Users spam search/reveal/flush so pad an extra position to reduce eye strain
    if (!affects.contains(CharacterAffectEnum::SEARCH)) {
        ++m_count;
    }
}

void GroupStateData::paint(QPainter *const pPainter, const QRect &rect)
{
    auto &painter = deref(pPainter);
    painter.fillRect(rect, m_color);

    painter.save();
    painter.translate(rect.x(), rect.y());
    m_height = rect.height();
    painter.scale(m_height, m_height); // Images are squares

    const bool invert = mmqt::textColor(m_color) == Qt::white;

    const auto drawOne = [&painter, invert](QString filename) -> void {
        painter.drawImage(QRect{0, 0, 1, 1}, getImage(filename, invert));
        painter.translate(1, 0);
    };

    if (m_position != CharacterPositionEnum::UNDEFINED) {
        drawOne(getIconFilename(m_position));
    }
    for (const CharacterAffectEnum affect : ALL_CHARACTER_AFFECTS) {
        if (m_affects.contains(affect)) {
            drawOne(getIconFilename(affect));
        }
    }
    painter.restore();
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
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        const int padding = size.width() / 2;
        const int content = stateData.getWidth();
        size.setWidth(padding + content);
        return size;
    }
    return QStyledItemDelegate::sizeHint(option, index);
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

NODISCARD static QString calculatePercentage(const int numerator, const int denomenator)
{
    if (denomenator == 0) {
        return "";
    }
    int percentage = static_cast<int>(100.0 * static_cast<double>(numerator)
                                      / static_cast<double>(denomenator));
    // QT documentation doesn't say it's legal to use "\\%" or "%%", so we'll just append.
    return QString("%1").arg(percentage).append("%");
}

NODISCARD static QString calculateRatio(const int numerator, const int denomenator)
{
    if (numerator == 0 && denomenator == 0) {
        return "";
    }
    return QString("%1/%2").arg(numerator).arg(denomenator);
}

NODISCARD static QString getPrettyName(const CharacterPositionEnum position)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterPositionEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (position) {
        XFOREACH_CHARACTER_POSITION(X_CASE)
    }
    return QString::asprintf("(CharacterPositionEnum)%d", static_cast<int>(position));
#undef X_CASE
}
NODISCARD static QString getPrettyName(const CharacterAffectEnum affect)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterAffectEnum::UPPER_CASE: \
        return friendly; \
    } while (false);
    switch (affect) {
        XFOREACH_CHARACTER_AFFECT(X_CASE)
    }
    return QString::asprintf("(CharacterAffectEnum)%d", static_cast<int>(affect));
#undef X_CASE
}

QVariant GroupModel::dataForCharacter(const SharedGroupChar &pCharacter,
                                      const ColumnTypeEnum column,
                                      const int role) const
{
    const CGroupChar &character = deref(pCharacter);
    // Map column to data
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case ColumnTypeEnum::NAME:
            if (character.getLabel().isEmpty() || character.getName() == character.getLabel()) {
                return character.getName();
            } else {
                return QString("%1 (%2)").arg(QString::fromLatin1(character.getName()),
                                              QString::fromLatin1(character.getLabel()));
            }
        case ColumnTypeEnum::HP_PERCENT:
            return calculatePercentage(character.hp, character.maxhp);
        case ColumnTypeEnum::MANA_PERCENT:
            return calculatePercentage(character.mana, character.maxmana);
        case ColumnTypeEnum::MOVES_PERCENT:
            return calculatePercentage(character.moves, character.maxmoves);
        case ColumnTypeEnum::HP:
            return calculateRatio(character.hp, character.maxhp);
        case ColumnTypeEnum::MANA:
            return calculateRatio(character.mana, character.maxmana);
        case ColumnTypeEnum::MOVES:
            return calculateRatio(character.moves, character.maxmoves);
        case ColumnTypeEnum::STATE:
            return QVariant::fromValue(
                GroupStateData(character.getColor(), character.position, character.affects));
        case ColumnTypeEnum::ROOM_NAME:
            if (character.roomId != INVALID_ROOMID && !m_map->isEmpty() && m_mapLoaded
                && character.roomId <= m_map->getMaxId()) {
                auto roomSelection = RoomSelection(*m_map);
                if (const Room *const r = roomSelection.getRoom(character.roomId)) {
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
        return character.getColor();

    case Qt::ForegroundRole:
        return mmqt::textColor(character.getColor());

    case Qt::TextAlignmentRole:
        if (column != ColumnTypeEnum::NAME && column != ColumnTypeEnum::ROOM_NAME) {
            // NOTE: There's no QVariant(AlignmentFlag) constructor.
            return static_cast<int>(Qt::AlignCenter);
        }
        break;

    case Qt::ToolTipRole:
        switch (column) {
        case ColumnTypeEnum::HP_PERCENT:
            return calculateRatio(character.hp, character.maxhp);
        case ColumnTypeEnum::MANA_PERCENT:
            return calculateRatio(character.mana, character.maxmana);
        case ColumnTypeEnum::MOVES_PERCENT:
            return calculateRatio(character.moves, character.maxmoves);
        case ColumnTypeEnum::STATE: {
            QString prettyName = getPrettyName(character.position);
            for (const CharacterAffectEnum affect : ALL_CHARACTER_AFFECTS) {
                if (character.affects.contains(affect)) {
                    prettyName.append(", ").append(getPrettyName(affect));
                }
            }
            return prettyName;
        }
        case ColumnTypeEnum::NAME:
        case ColumnTypeEnum::HP:
        case ColumnTypeEnum::MANA:
        case ColumnTypeEnum::MOVES:
        case ColumnTypeEnum::ROOM_NAME:
            break;
        }
        break;

    default:
        break;
    }

    return QVariant();
}

QVariant GroupModel::data(const QModelIndex &index, int role) const
{
    QVariant data = QVariant();
    if (!index.isValid()) {
        return data;
    }

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
            slot_messageBox("Group Manager", QString::fromLatin1(ex.what()));
        }
    });

    connect(m_table, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        if (!index.isValid()) {
            return;
        }

        // Identify target
        if (auto *const g = m_group->getGroup()) {
            auto selection = g->selectAll();
            // Map row to character
            if (index.row() < static_cast<int>(selection->size())) {
                const SharedGroupChar &pCharacter = selection->at(index.row());
                const CGroupChar &character = deref(pCharacter);
                selectedCharacter = character.getName();

                // Center map on the clicked character
                if (character.roomId != INVALID_ROOMID && !m_map->isEmpty()
                    && character.roomId <= m_map->getMaxId()) {
                    auto roomSelection = RoomSelection(*m_map);
                    if (const Room *const r = roomSelection.getRoom(character.roomId)) {
                        const Coordinate &c = r->getPosition();
                        const auto worldPos = c.to_vec2() + glm::vec2{0.5f, 0.5f};
                        emit sig_center(worldPos); // connects to MapWindow
                    }
                }
            }

            // Build Context menu
            const bool isServer = Mmapper2Group::getConfigState() == GroupManagerStateEnum::Server;
            const bool selectedSelf = (index.row() == 0);
            if (isServer && !selectedSelf) {
                // All context menu actions are only actionable by the server right now
                m_kick->setText(QString("&Kick %1").arg(QString::fromLatin1(selectedCharacter)));
                QMenu contextMenu(tr("Context menu"), this);
                contextMenu.addAction(m_kick);
                contextMenu.exec(QCursor::pos());
            }
        }
    });

    connect(m_group,
            &Mmapper2Group::sig_updateWidget,
            this,
            &GroupWidget::slot_updateLabels,
            Qt::QueuedConnection);
    connect(m_group,
            &Mmapper2Group::sig_messageBox,
            this,
            &GroupWidget::slot_messageBox,
            Qt::QueuedConnection);

    readSettings();
}

GroupWidget::~GroupWidget()
{
    delete m_table;
    delete m_kick;
    writeSettings();
}

void GroupWidget::slot_updateLabels()
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

void GroupWidget::slot_messageBox(const QString &title, const QString &message)
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
