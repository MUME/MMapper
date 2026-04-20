// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupwidget.h"

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../global/Timer.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "CGroupChar.h"
#include "enums.h"
#include "mmapper2group.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

#include <QAction>
#include <QColorDialog>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QMessageLogContext>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

static constexpr const char *GROUP_MIME_TYPE = "application/vnd.mm_groupchar.row";

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

NODISCARD static const char *getColumnFriendlyName(const ColumnTypeEnum column)
{
#define X_DECL_COLUMNTYPE(UPPER_CASE, lower_case, CamelCase, friendly) \
    case ColumnTypeEnum::UPPER_CASE: \
        return friendly;

    switch (column) {
        XFOREACH_COLUMNTYPE(X_DECL_COLUMNTYPE)
    default:
        qWarning() << "Unsupported column type" << static_cast<int>(column);
        return "";
    }
#undef X_DECL_COLUMNTYPE
}
} // namespace

GroupProxyModel::GroupProxyModel(QObject *const parent)
    : QSortFilterProxyModel(parent)
{}

GroupProxyModel::~GroupProxyModel() = default;

void GroupProxyModel::refresh()
{
    // Triggers re-filter and re-sort
    invalidate();
}

SharedGroupChar GroupProxyModel::getCharacterFromSource(const QModelIndex &source_index) const
{
    const GroupModel *srcModel = qobject_cast<const GroupModel *>(sourceModel());
    if (!srcModel || !source_index.isValid()) {
        return nullptr;
    }

    return srcModel->getCharacter(source_index.row());
}

bool GroupProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!getConfig().groupManager.npcHide) {
        return true;
    }

    QModelIndex sourceIndex = sourceModel()->index(source_row, 0, source_parent);
    SharedGroupChar character = getCharacterFromSource(sourceIndex);
    if (character && character->isNpc()) {
        return false;
    }

    return true;
}

GroupStateData::GroupStateData(const QColor &color,
                               const CharacterPositionEnum position,
                               const CharacterAffectFlags affects)
    : m_color(color)
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
    if (!affects.contains(CharacterAffectEnum::WAITING)) {
        ++m_count;
    }
}

void GroupStateData::paint(QPainter *const pPainter, const QRect &rect)
{
    auto &painter = deref(pPainter);

    painter.save();
    painter.translate(rect.x(), rect.y());
    m_height = rect.height();
    painter.scale(static_cast<double>(m_height),
                  static_cast<double>(m_height)); // Images are squares

    const bool invert = mmqt::textColor(m_color) == Qt::white;

    const auto drawOne = [&painter, invert](QString filename) -> void {
        painter.drawImage(QRect{0, 0, 1, 1}, getImage(filename, invert));
        painter.translate(1.0, 0.0);
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

GroupDelegate::GroupDelegate(QObject *const parent)
    : QStyledItemDelegate(parent)
{}

GroupDelegate::~GroupDelegate() = default;

void GroupDelegate::paint(QPainter *const pPainter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    auto &painter = deref(pPainter);

    const auto column = static_cast<ColumnTypeEnum>(index.column());
    const GroupModel *model = qobject_cast<const GroupModel *>(index.model());
    if (!model) {
        // Handle proxy model
        const auto *proxy = qobject_cast<const QSortFilterProxyModel *>(index.model());
        if (proxy) {
            model = qobject_cast<const GroupModel *>(proxy->sourceModel());
        }
    }

    SharedGroupChar character = nullptr;
    if (model) {
        const auto sourceIndex = (index.model() == model)
                                     ? index
                                     : qobject_cast<const QSortFilterProxyModel *>(index.model())
                                           ->mapToSource(index);
        character = model->getCharacter(sourceIndex.row());
    }

    if (!character) {
        QStyledItemDelegate::paint(pPainter, option, index);
        return;
    }

    const QRect rect = option.rect;
    const QColor charColor = character->getColor();

    // Layer 0: Background
    painter.fillRect(rect, charColor);

    // Selection highlight (subtle overlay)
    if (option.state & QStyle::State_Selected) {
        QColor selectColor = option.palette.color(QPalette::Highlight);
        selectColor.setAlpha(60);
        painter.fillRect(rect, selectColor);
    }

    if (index.data().canConvert<GroupStateData>()) {
        GroupStateData stateData = qvariant_cast<GroupStateData>(index.data());
        stateData.paint(pPainter, option.rect);
        return;
    }

    const QColor textColor = mmqt::textColor(charColor);

    if (column == ColumnTypeEnum::NAME || column == ColumnTypeEnum::ROOM_NAME) {
        painter.save();
        painter.setPen(textColor);
        painter.drawText(rect.adjusted(4, 0, 0, 0),
                         static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft),
                         index.data(Qt::DisplayRole).toString());
        painter.restore();

    } else if (column == ColumnTypeEnum::HP || column == ColumnTypeEnum::MANA
               || column == ColumnTypeEnum::MOVES) {
        int cur = 0;
        int max = 0;
        QColor barColor;
        bool pulse = false;
        int pulseMin = 100;
        int pulseMax = 180;

        if (column == ColumnTypeEnum::HP) {
            cur = character->getHits();
            max = character->getMaxHits();
            const double pct = max > 0 ? static_cast<double>(cur) / max : 1.0;
            if (pct < 0.3) {
                barColor = QColor(0xFF, 0x55, 0x55); // Red
                pulse = true;
                pulseMax = 255;
            } else {
                barColor = QColor(0x50, 0xFA, 0x7B); // Green
            }
        } else if (column == ColumnTypeEnum::MANA) {
            cur = character->getMana();
            max = character->getMaxMana();
            if (max <= 0) {
                // Hidden State
                painter.save();
                painter.setPen(option.palette.color(QPalette::Text));
                painter.drawText(rect, Qt::AlignCenter, "--");
                painter.restore();
                return;
            }
            barColor = QColor(0x8B, 0xE9, 0xFD); // Cyan
        } else if (column == ColumnTypeEnum::MOVES) {
            cur = character->getMoves();
            max = character->getMaxMoves();
            barColor = QColor(0xFF, 0xB8, 0x6C); // Amber
            const double pct = max > 0 ? static_cast<double>(cur) / max : 1.0;
            if (pct < 0.15) {
                pulse = true;
            }
        }

        // Layer 2: The Bar (80% height, rounded 4px)
        const int barHeight = static_cast<int>(static_cast<double>(rect.height()) * 0.8);
        const int barY = rect.y() + (rect.height() - barHeight) / 2;
        const double pct = std::clamp(max > 0 ? static_cast<double>(cur) / max : 0.0, 0.0, 1.0);
        const int barWidth = static_cast<int>(static_cast<double>(std::max(0, rect.width() - 2))
                                              * pct);
        const QRect barRect(rect.x() + 1, barY, std::max(0, rect.width() - 2), barHeight);

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);

        // Bar background and thin black border
        painter.setBrush(option.palette.color(QPalette::Window));
        painter.setPen(QPen(Qt::black, 1));
        painter.drawRoundedRect(barRect, 4.0, 4.0);

        // Bar foreground (the actual progress)
        if (barWidth > 0) {
            int alpha = 180;
            if (pulse) {
                // 1.5s cycle (1500ms)
                static constexpr const double PI = 3.14159265358979323846;
                const qint64 ms = QDateTime::currentMSecsSinceEpoch() % 1500;
                const double phase = (std::sin((static_cast<double>(ms) / 1500.0) * 2.0 * PI
                                               - PI / 2.0)
                                      + 1.0)
                                     / 2.0;
                alpha = pulseMin
                        + static_cast<int>(
                            std::round(static_cast<double>(pulseMax - pulseMin) * phase));
            }
            barColor.setAlpha(alpha);

            painter.setBrush(barColor);
            painter.setPen(Qt::NoPen);
            // Clip to progress width to keep the rounded corners of the container
            painter.setClipRect(rect.x() + 1, barY, barWidth, barHeight, Qt::IntersectClip);
            painter.drawRoundedRect(barRect, 4.0, 4.0);
        }
        painter.restore();

        // Layer 3: Text (Monospace, Centered)
        painter.save();
        QFont monoFont = option.font;
        monoFont.setFamily("DejaVu Sans Mono");
        monoFont.setStyleHint(QFont::Monospace);
        painter.setFont(monoFont);

        // Use text color appropriate for QPalette::Window (the bar container background)
        painter.setPen(mmqt::textColor(option.palette.color(QPalette::Window)));

        painter.drawText(rect,
                         static_cast<int>(Qt::AlignCenter),
                         index.data(Qt::DisplayRole).toString());
        painter.restore();
    } else {
        // Other columns
        painter.save();
        painter.setPen(textColor);
        painter.drawText(rect,
                         static_cast<int>(Qt::AlignCenter),
                         index.data(Qt::DisplayRole).toString());
        painter.restore();
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

    const auto column = static_cast<ColumnTypeEnum>(index.column());
    if (column == ColumnTypeEnum::HP || column == ColumnTypeEnum::MANA
        || column == ColumnTypeEnum::MOVES) {
        QFont monoFont = option.font;
        monoFont.setFamily("DejaVu Sans Mono");
        monoFont.setStyleHint(QFont::Monospace);
        const QFontMetrics fm(monoFont);

        const int textWidth = fm.horizontalAdvance(QStringLiteral("999 / 999"));
        return QSize(textWidth + 20, QStyledItemDelegate::sizeHint(option, index).height());
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

GroupModel::GroupModel(QObject *const parent)
    : QAbstractTableModel(parent)
{}

void GroupModel::setCharacters(const GroupVector &newGameChars)
{
    DECL_TIMER(t, __FUNCTION__);

    std::unordered_set<GroupId> newGameCharIds;
    newGameCharIds.reserve(newGameChars.size());
    for (const auto &pGameChar : newGameChars) {
        const auto &gameChar = deref(pGameChar);
        newGameCharIds.insert(gameChar.getId());
    }

    GroupVector resultingCharacterList;
    resultingCharacterList.reserve(m_characters.size() + newGameChars.size());

    // Temporary vectors to hold truly new players, NPCs, and all truly new characters
    GroupVector trulyNewPlayers;
    trulyNewPlayers.reserve(newGameChars.size());
    GroupVector trulyNewNpcs;
    trulyNewNpcs.reserve(newGameChars.size());
    GroupVector allTrulyNewCharsInOriginalOrder;
    allTrulyNewCharsInOriginalOrder.reserve(newGameChars.size());

    // Preserve existing characters
    std::unordered_set<GroupId> existingIds;
    existingIds.reserve(m_characters.size());
    for (const auto &pExistingChar : m_characters) {
        const auto &existingChar = deref(pExistingChar);
        existingIds.insert(existingChar.getId());
        if (newGameCharIds.count(existingChar.getId())) {
            resultingCharacterList.push_back(pExistingChar);
        }
    }

    // Identify truly new characters and categorize them as NPC or player
    for (const auto &pGameChar : newGameChars) {
        const auto &gameChar = deref(pGameChar);
        if (existingIds.find(gameChar.getId()) == existingIds.end()) {
            allTrulyNewCharsInOriginalOrder.push_back(pGameChar);
            if (gameChar.isNpc()) {
                trulyNewNpcs.push_back(pGameChar);
            } else {
                trulyNewPlayers.push_back(pGameChar);
            }
        }
    }

    // Insert the newly identified characters into the resulting list based on configuration.
    if (getConfig().groupManager.npcSortBottom) {
        // Find the insertion point for new players: before the first NPC in the preserved list.
        auto itPlayerInsertPos = resultingCharacterList.begin();
        while (itPlayerInsertPos != resultingCharacterList.end()) {
            if (*itPlayerInsertPos && (*itPlayerInsertPos)->isNpc()) {
                break;
            }
            ++itPlayerInsertPos;
        }

        // Insert truly new players at the determined position.
        if (!trulyNewPlayers.empty()) {
            resultingCharacterList.insert(itPlayerInsertPos,
                                          trulyNewPlayers.begin(),
                                          trulyNewPlayers.end());
        }
        // Insert truly new NPCs at the end of the list.
        if (!trulyNewNpcs.empty()) {
            resultingCharacterList.insert(resultingCharacterList.end(),
                                          trulyNewNpcs.begin(),
                                          trulyNewNpcs.end());
        }
    } else {
        // If no special NPC sorting, just append all truly new characters in their original order.
        if (!allTrulyNewCharsInOriginalOrder.empty()) {
            resultingCharacterList.insert(resultingCharacterList.end(),
                                          allTrulyNewCharsInOriginalOrder.begin(),
                                          allTrulyNewCharsInOriginalOrder.end());
        }
    }

    beginResetModel();
    m_characters = std::move(resultingCharacterList);
    endResetModel();
}

int GroupModel::findIndexById(const GroupId charId) const
{
    if (charId == INVALID_GROUPID) {
        return -1;
    }
    auto it = std::find_if(m_characters.begin(),
                           m_characters.end(),
                           [charId](const SharedGroupChar &c) { return c && c->getId() == charId; });
    if (it != m_characters.end()) {
        return static_cast<int>(std::distance(m_characters.begin(), it));
    }
    return -1;
}

void GroupModel::insertCharacter(const SharedGroupChar &newCharacter)
{
    assert(newCharacter);
    if (newCharacter->getId() == INVALID_GROUPID) {
        return;
    }

    auto calculateInsertIndex = [this, &newCharacter]() -> int {
        if (getConfig().groupManager.npcSortBottom && !newCharacter->isNpc()) {
            auto it = std::find_if(m_characters.begin(),
                                   m_characters.end(),
                                   [](const SharedGroupChar &c) { return c && c->isNpc(); });
            return static_cast<int>(std::distance(m_characters.begin(), it));
        }
        return static_cast<int>(m_characters.size());
    };

    const int newIndex = calculateInsertIndex();
    assert(newIndex >= 0 && newIndex <= static_cast<int>(m_characters.size()));

    beginInsertRows(QModelIndex(), newIndex, newIndex);
    m_characters.insert(m_characters.begin() + newIndex, newCharacter);
    endInsertRows();
}

void GroupModel::removeCharacterById(const GroupId charId)
{
    const int index = findIndexById(charId);
    if (index == -1) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_characters.erase(m_characters.begin() + index);
    endRemoveRows();
}

void GroupModel::updateCharacter(const SharedGroupChar &updatedCharacter)
{
    assert(updatedCharacter);
    const GroupId charId = updatedCharacter->getId();
    const int index = findIndexById(charId);
    if (index == -1) {
        insertCharacter(updatedCharacter);
        return;
    }

    SharedGroupChar &existingChar = m_characters[static_cast<size_t>(index)];
    existingChar = updatedCharacter;

    emit dataChanged(this->index(index, 0),
                     this->index(index, columnCount(QModelIndex()) - 1),
                     {Qt::DisplayRole,
                      Qt::BackgroundRole,
                      Qt::ForegroundRole,
                      Qt::ToolTipRole,
                      Qt::UserRole + 1});
}

SharedGroupChar GroupModel::getCharacter(int row) const
{
    if (row >= 0 && row < static_cast<int>(m_characters.size())) {
        return m_characters.at(static_cast<size_t>(row));
    }
    return nullptr;
}

void GroupModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

int GroupModel::rowCount(const QModelIndex & /* parent */) const
{
    return static_cast<int>(m_characters.size());
}

int GroupModel::columnCount(const QModelIndex & /* parent */) const
{
    return GROUP_COLUMN_COUNT;
}

NODISCARD static QString getPrettyName(const CharacterPositionEnum position)
{
#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
    case CharacterPositionEnum::UPPER_CASE: \
        return QStringLiteral(friendly); \
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
        return QStringLiteral(friendly); \
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

    const auto formatStat = [&character](int numerator, int denomenator) -> QString {
        if (numerator == 0 && denomenator == 0) {
            return QLatin1String("");
        }

        if (character.getType() == CharacterTypeEnum::NPC) {
            const double pct = denomenator > 0 ? static_cast<double>(numerator) / denomenator : 0.0;
            return QString("%1%").arg(static_cast<int>(std::round(pct * 100.0)));
        }

        return QString("%1 / %2").arg(numerator).arg(denomenator);
    };

    // Map column to data
    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case ColumnTypeEnum::NAME:
            if (character.getLabel().isEmpty()
                || character.getName().getStdStringViewUtf8()
                       == character.getLabel().getStdStringViewUtf8()) {
                return character.getName().toQString();
            } else {
                return QString("%1 (%2)").arg(character.getName().toQString(),
                                              character.getLabel().toQString());
            }
        case ColumnTypeEnum::HP:
            return formatStat(character.getHits(), character.getMaxHits());
        case ColumnTypeEnum::MANA:
            return formatStat(character.getMana(), character.getMaxMana());
        case ColumnTypeEnum::MOVES:
            return formatStat(character.getMoves(), character.getMaxMoves());
        case ColumnTypeEnum::STATE:
            return QVariant::fromValue(GroupStateData(character.getColor(),
                                                      character.getPosition(),
                                                      character.getAffects()));
        case ColumnTypeEnum::ROOM_NAME:
            if (character.getRoomName().isEmpty()) {
                return QStringLiteral("Somewhere");
            } else {
                return character.getRoomName().toQString();
            }
        default:
            qWarning() << "Unsupported column" << static_cast<int>(column);
            break;
        }
        break;

    case Qt::BackgroundRole:
        return QVariant();

    case Qt::ForegroundRole:
        return QVariant();

    case Qt::TextAlignmentRole:
        if (column != ColumnTypeEnum::NAME && column != ColumnTypeEnum::ROOM_NAME) {
            // NOTE: There's no QVariant(AlignmentFlag) constructor.
            return static_cast<int>(Qt::AlignCenter);
        }
        break;

    case Qt::ToolTipRole: {
        switch (column) {
        case ColumnTypeEnum::HP:
        case ColumnTypeEnum::MANA:
        case ColumnTypeEnum::MOVES: {
            const int cur = (column == ColumnTypeEnum::HP)     ? character.getHits()
                            : (column == ColumnTypeEnum::MANA) ? character.getMana()
                                                               : character.getMoves();
            const int max = (column == ColumnTypeEnum::HP)     ? character.getMaxHits()
                            : (column == ColumnTypeEnum::MANA) ? character.getMaxMana()
                                                               : character.getMaxMoves();
            if (max > 0) {
                const double pct = static_cast<double>(cur) / max;
                return QString("%1%").arg(static_cast<int>(std::round(pct * 100.0)));
            }
            return QVariant();
        }
        case ColumnTypeEnum::STATE: {
            QString prettyName;
            prettyName += getPrettyName(character.getPosition());
            for (const CharacterAffectEnum affect : ALL_CHARACTER_AFFECTS) {
                if (character.getAffects().contains(affect)) {
                    prettyName.append(QStringLiteral(", ")).append(getPrettyName(affect));
                }
            }
            return prettyName;
        }
        case ColumnTypeEnum::NAME:
            break;
        case ColumnTypeEnum::ROOM_NAME:
            if (character.getServerId() != INVALID_SERVER_ROOMID) {
                return QString("%1").arg(character.getServerId().asUint32());
            }
            break;
        }
        break;
    }

    default:
        break;
    }

    return QVariant();
}

QVariant GroupModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= 0 && index.row() < static_cast<int>(m_characters.size())) {
        const SharedGroupChar &character = m_characters.at(static_cast<size_t>(index.row()));
        return dataForCharacter(character, static_cast<ColumnTypeEnum>(index.column()), role);
    }

    return QVariant();
}

QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Orientation::Horizontal) {
            return getColumnFriendlyName(static_cast<ColumnTypeEnum>(section));
        }
        break;
    }
    return QVariant();
}

Qt::ItemFlags GroupModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions GroupModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList GroupModel::mimeTypes() const
{
    QStringList types;
    types << GROUP_MIME_TYPE;
    return types;
}

QMimeData *GroupModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeDataObj = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    if (!indexes.isEmpty() && indexes.first().isValid()) {
        stream << indexes.first().row();
    }
    mimeDataObj->setData(GROUP_MIME_TYPE, encodedData);
    return mimeDataObj;
}

bool GroupModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (!data->hasFormat(GROUP_MIME_TYPE) || column > 0) {
        return false;
    }

    QByteArray encodedData = data->data(GROUP_MIME_TYPE);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    if (stream.atEnd()) {
        return false;
    }
    int sourceRow;
    stream >> sourceRow;

    int targetInsertionIndex;
    if (parent.isValid()) {
        targetInsertionIndex = parent.row();
    } else if (row != -1) {
        targetInsertionIndex = row;
    } else {
        targetInsertionIndex = static_cast<int>(m_characters.size());
    }

    if (sourceRow == targetInsertionIndex
        || (sourceRow == targetInsertionIndex - 1 && targetInsertionIndex > sourceRow)) {
        return false;
    }

    if (targetInsertionIndex < 0) {
        targetInsertionIndex = 0;
    }
    if (targetInsertionIndex > static_cast<int>(m_characters.size())) {
        targetInsertionIndex = static_cast<int>(m_characters.size());
    }

    if (!beginMoveRows(QModelIndex(), sourceRow, sourceRow, QModelIndex(), targetInsertionIndex)) {
        return false;
    }

    SharedGroupChar movedChar = m_characters[static_cast<size_t>(sourceRow)];
    m_characters.erase(m_characters.begin() + sourceRow);

    int actualInsertionIdx = targetInsertionIndex;
    if (sourceRow < targetInsertionIndex) {
        actualInsertionIdx--;
    }

    if (actualInsertionIdx < 0) {
        actualInsertionIdx = 0;
    }
    if (actualInsertionIdx > static_cast<int>(m_characters.size())) {
        actualInsertionIdx = static_cast<int>(m_characters.size());
    }

    m_characters.insert(m_characters.begin() + actualInsertionIdx, movedChar);

    endMoveRows();
    return true;
}

GroupWidget::GroupWidget(Mmapper2Group *const group, MapData *const md, QWidget *const parent)
    : QWidget(parent)
    , m_group(group)
    , m_map(md)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_table = new QTableView(this);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_model = new GroupModel(m_table);
    if (m_group) {
        m_model->setCharacters(m_group->selectAll());
    } else {
        m_model->setCharacters({});
    }

    m_proxyModel = new GroupProxyModel(m_table);
    m_proxyModel->setSourceModel(m_model);
    m_table->setModel(m_proxyModel);

    m_table->setDragEnabled(true);
    m_table->setAcceptDrops(true);
    m_table->setDragDropMode(QAbstractItemView::InternalMove);
    m_table->setDefaultDropAction(Qt::MoveAction);
    m_table->setDropIndicatorShown(true);

    m_table->setItemDelegate(new GroupDelegate(m_table));
    layout->addWidget(m_table);

    m_pulseTimer = new QTimer(this);
    connect(m_pulseTimer, &QTimer::timeout, m_table->viewport(), QOverload<>::of(&QWidget::update));

    // Minimize row height
    m_table->verticalHeader()->setDefaultSectionSize(
        m_table->verticalHeader()->minimumSectionSize());

    m_center = new QAction(QIcon(":/icons/roomfind.png"), tr("&Center"), this);
    connect(m_center, &QAction::triggered, this, [this]() {
        // Center map on the clicked character
        auto &character = deref(selectedCharacter);
        if (character.isYou()) {
            if (const auto &r = m_map->getCurrentRoom()) {
                const auto vec2 = r.getPosition().to_vec2() + glm::vec2{0.5f, 0.5f};
                emit sig_center(vec2); // connects to MapWindow
                return;
            }
        }
        const ServerRoomId srvId = character.getServerId();
        if (srvId != INVALID_SERVER_ROOMID) {
            if (const auto &r = m_map->findRoomHandle(srvId)) {
                const auto vec2 = r.getPosition().to_vec2() + glm::vec2{0.5f, 0.5f};
                emit sig_center(vec2); // connects to MapWindow
            }
        }
    });

    m_recolor = new QAction(QIcon(":/icons/group-recolor.png"), tr("&Recolor"), this);
    connect(m_recolor, &QAction::triggered, this, [this]() {
        auto &character = deref(selectedCharacter);
        const QColor newColor = QColorDialog::getColor(character.getColor(), this);
        if (newColor.isValid() && newColor != character.getColor()) {
            character.setColor(newColor);
            if (selectedCharacter->isYou()) {
                setConfig().groupManager.color = newColor;
            }
        }
    });

    connect(m_table, &QAbstractItemView::clicked, this, [this](const QModelIndex &proxyIndex) {
        if (!proxyIndex.isValid()) {
            return;
        }

        QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);

        if (!sourceIndex.isValid()) {
            return;
        }

        selectedCharacter = deref(m_model).getCharacter(sourceIndex.row());
        if (selectedCharacter) {
            // Build Context menu
            m_center->setText(
                QString("&Center on %1").arg(selectedCharacter->getName().toQString()));
            m_recolor->setText(QString("&Recolor %1").arg(selectedCharacter->getName().toQString()));
            m_center->setDisabled(!selectedCharacter->isYou()
                                  && selectedCharacter->getServerId() == INVALID_SERVER_ROOMID);

            auto *contextMenu = new QMenu(tr("Context menu"), this);
            contextMenu->setAttribute(Qt::WA_DeleteOnClose);
            contextMenu->addAction(m_center);
            contextMenu->addAction(m_recolor);
            contextMenu->popup(QCursor::pos());
        }
    });

    connect(m_group, &Mmapper2Group::sig_characterAdded, this, &GroupWidget::slot_onCharacterAdded);
    connect(m_group,
            &Mmapper2Group::sig_characterRemoved,
            this,
            &GroupWidget::slot_onCharacterRemoved);
    connect(m_group,
            &Mmapper2Group::sig_characterUpdated,
            this,
            &GroupWidget::slot_onCharacterUpdated);
    connect(m_group, &Mmapper2Group::sig_groupReset, this, &GroupWidget::slot_onGroupReset);

    updateColumnVisibility();
    updatePulseTimer();
}

GroupWidget::~GroupWidget()
{
    m_pulseTimer->stop();
}

QSize GroupWidget::sizeHint() const
{
    int headerHeight = m_table->horizontalHeader()->height();
    int rowHeight = m_table->verticalHeader()->minimumSectionSize();
    int desiredHeight = headerHeight + rowHeight + (m_table->frameWidth() * 2);
    int preferredWidth = m_table->horizontalHeader()->length();
    return QSize(preferredWidth, desiredHeight);
}

void GroupWidget::updateColumnVisibility()
{
    // Hide unnecessary columns like mana if everyone is a zorc/troll
    const auto one_character_had_mana = [this]() -> bool {
        for (const auto &character : deref(m_model).getCharacters()) {
            if (character && (character->getMana() > 0 || character->getMaxMana() > 0)) {
                return true;
            }
        }
        return false;
    };
    const bool hide_mana = !one_character_had_mana();
    m_table->setColumnHidden(static_cast<int>(ColumnTypeEnum::MANA), hide_mana);
}

void GroupWidget::updatePulseTimer()
{
    const auto needs_pulse = [this]() -> bool {
        for (const auto &character : deref(m_model).getCharacters()) {
            if (!character) {
                continue;
            }
            // HP pulse < 30%
            const int hp = character->getHits();
            const int maxHp = character->getMaxHits();
            if (maxHp > 0 && (static_cast<double>(hp) / maxHp) < 0.3) {
                return true;
            }
            // Moves pulse < 15%
            const int moves = character->getMoves();
            const int maxMoves = character->getMaxMoves();
            if (maxMoves > 0 && (static_cast<double>(moves) / maxMoves) < 0.15) {
                return true;
            }
        }
        return false;
    };

    if (needs_pulse()) {
        if (!m_pulseTimer->isActive()) {
            m_pulseTimer->start(50);
        }
    } else {
        if (m_pulseTimer->isActive()) {
            m_pulseTimer->stop();
        }
    }
}

void GroupWidget::slot_onCharacterAdded(SharedGroupChar character)
{
    assert(character);
    deref(m_model).insertCharacter(character);
    updateColumnVisibility();
    updatePulseTimer();
}

void GroupWidget::slot_onCharacterRemoved(const GroupId characterId)
{
    assert(characterId != INVALID_GROUPID);
    deref(m_model).removeCharacterById(characterId);
    updateColumnVisibility();
    updatePulseTimer();
}

void GroupWidget::slot_onCharacterUpdated(SharedGroupChar character)
{
    assert(character);
    deref(m_model).updateCharacter(character);
    updatePulseTimer();
}

void GroupWidget::slot_onGroupReset(const GroupVector &newCharacterList)
{
    deref(m_model).setCharacters(newCharacterList);
    updateColumnVisibility();
    updatePulseTimer();
}
