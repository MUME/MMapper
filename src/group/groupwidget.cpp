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
#include "display/GhostRegistry.h"
#include "enums.h"
#include "mmapper2group.h"
#include "tokenmanager.h"

#include <algorithm>
#include <map>
#include <vector>

#include <QAction>
#include <QColorDialog>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

extern const QString kForceFallback;

static_assert(GROUP_COLUMN_COUNT == static_cast<int>(ColumnTypeEnum::ROOM_NAME) + 1, "# of columns");

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
    if (!affects.contains(CharacterAffectEnum::WAITING)) {
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

GroupDelegate::GroupDelegate(QObject *const parent)
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
        QStyleOptionViewItem opt = option;
        opt.state &= ~QStyle::State_HasFocus;
        opt.state &= ~QStyle::State_Selected;

        QStyledItemDelegate::paint(painter, opt, index);
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

GroupModel::GroupModel(QObject *const parent)
    : QAbstractTableModel(parent)
{}

namespace { // anonymous – helpers local to this file
static void insertNewCharactersInto(GroupVector &dest,
                                    bool npcSortBottom,
                                    const GroupVector &newPlayers,
                                    const GroupVector &newNpcs,
                                    const GroupVector &newAll)
{
    auto eraseGhosts = [](const GroupVector &vec) {
        if (!getConfig().groupManager.showNpcGhosts)
            return;
        for (const auto &p : vec)
            g_ghosts.erase(deref(p).getServerId());
    };
    if (npcSortBottom) {
        // players go before first NPC already present
        auto firstNpc = std::find_if(dest.begin(), dest.end(), [](const SharedGroupChar &c) {
            return c && c->isNpc();
        });
        dest.insert(firstNpc, newPlayers.begin(), newPlayers.end());
        eraseGhosts(newPlayers);
        dest.insert(dest.end(), newNpcs.begin(), newNpcs.end());
        eraseGhosts(newNpcs);
    } else {
        dest.insert(dest.end(), newAll.begin(), newAll.end());
        eraseGhosts(newAll);
    }
}
} // namespace

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

        if (getConfig().groupManager.showNpcGhosts)
            g_ghosts.erase(gameChar.getServerId());

        if (existingIds.find(gameChar.getId()) == existingIds.end()) {
            allTrulyNewCharsInOriginalOrder.push_back(pGameChar);
            if (gameChar.isNpc()) {
                trulyNewNpcs.push_back(pGameChar);
            } else {
                trulyNewPlayers.push_back(pGameChar);
            }
        }
    }

    insertNewCharactersInto(resultingCharacterList,
                            getConfig().groupManager.npcSortBottom,
                            trulyNewPlayers,
                            trulyNewNpcs,
                            allTrulyNewCharsInOriginalOrder);

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
    if (index == -1)
        return;

    SharedGroupChar &c = m_characters[static_cast<size_t>(index)];

    /*** NEW: store a ghost entry if this row is a mount ***/
    if (getConfig().groupManager.showNpcGhosts && c->isNpc()) {
        g_ghosts[c->getServerId()] = {c->getDisplayName()};
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

NODISCARD static QStringView getPrettyName(const CharacterPositionEnum position)
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
NODISCARD static QStringView getPrettyName(const CharacterAffectEnum affect)
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

/*─────────────────────  complexity helpers  ─────────────────────*/
namespace {

static QString formatStatHelper(int num, int den, ColumnTypeEnum col, bool isNpc)
{
    if (col == ColumnTypeEnum::HP_PERCENT || col == ColumnTypeEnum::MANA_PERCENT
        || col == ColumnTypeEnum::MOVES_PERCENT) {
        if (den == 0)
            return {};
        int pct = static_cast<int>(100.0 * double(num) / double(den));
        return QString("%1%").arg(pct);
    }

    if (col == ColumnTypeEnum::HP || col == ColumnTypeEnum::MANA || col == ColumnTypeEnum::MOVES) {
        // hide “0/0” for NPCs -- same behaviour as before
        if (isNpc && num == 0 && den == 0)
            return {};
        return QString("%1/%2").arg(num).arg(den);
    }
    return {};
}

/// Big switch for DisplayRole, extracted out of dataForCharacter
static QVariant makeDisplayRole(const CGroupChar &ch, ColumnTypeEnum c, TokenManager *tm)
{
    switch (c) {
    case ColumnTypeEnum::CHARACTER_TOKEN:
        return tm ? QIcon(tm->getToken(ch.getDisplayName())) : QVariant();
    case ColumnTypeEnum::NAME:
        if (ch.getLabel().isEmpty()
            || ch.getName().getStdStringViewUtf8() == ch.getLabel().getStdStringViewUtf8()) {
            return ch.getName().toQString();
        } else {
            return QString("%1 (%2)").arg(ch.getName().toQString(), ch.getLabel().toQString());
        }
    case ColumnTypeEnum::HP_PERCENT:
        return formatStatHelper(ch.getHits(), ch.getMaxHits(), c, false);
    case ColumnTypeEnum::MANA_PERCENT:
        return formatStatHelper(ch.getMana(), ch.getMaxMana(), c, false);
    case ColumnTypeEnum::MOVES_PERCENT:
        return formatStatHelper(ch.getMoves(), ch.getMaxMoves(), c, false);
    case ColumnTypeEnum::HP:
        return formatStatHelper(ch.getHits(),
                                ch.getMaxHits(),
                                c,
                                ch.getType() == CharacterTypeEnum::NPC);
    case ColumnTypeEnum::MANA:
        return formatStatHelper(ch.getMana(),
                                ch.getMaxMana(),
                                c,
                                ch.getType() == CharacterTypeEnum::NPC);
    case ColumnTypeEnum::MOVES:
        return formatStatHelper(ch.getMoves(),
                                ch.getMaxMoves(),
                                c,
                                ch.getType() == CharacterTypeEnum::NPC);
    case ColumnTypeEnum::STATE:
        return QVariant::fromValue(GroupStateData(ch.getColor(), ch.getPosition(), ch.getAffects()));
    case ColumnTypeEnum::ROOM_NAME:
        return ch.getRoomName().isEmpty() ? QStringLiteral("Somewhere")
                                          : ch.getRoomName().toQString();
    default:
        return QVariant();
    }
}

/// Big switch for ToolTipRole, extracted out as well
static QVariant makeTooltipRole(const CGroupChar &ch, ColumnTypeEnum c, bool useStatFmt)
{
    auto statTip = [&](int n, int d) -> QVariant {
        return useStatFmt ? formatStatHelper(n, d, c, false) : QVariant();
    };

    switch (c) {
    case ColumnTypeEnum::CHARACTER_TOKEN:
        return QVariant();
    case ColumnTypeEnum::HP_PERCENT:
        return statTip(ch.getHits(), ch.getMaxHits());
    case ColumnTypeEnum::MANA_PERCENT:
        return statTip(ch.getMana(), ch.getMaxMana());
    case ColumnTypeEnum::MOVES_PERCENT:
        return statTip(ch.getMoves(), ch.getMaxMoves());
    case ColumnTypeEnum::STATE: {
        QString pretty = getPrettyName(ch.getPosition()).toString();
        for (const CharacterAffectEnum a : ALL_CHARACTER_AFFECTS)
            if (ch.getAffects().contains(a))
                pretty.append(", ").append(getPrettyName(a));
        return pretty;
    }
    case ColumnTypeEnum::ROOM_NAME:
        if (ch.getServerId() != INVALID_SERVER_ROOMID)
            return QString::number(ch.getServerId().asUint32());
        return QVariant();
    default:
        return QVariant();
    }
}

} // anonymous namespace
/*──────────────────────────────────────────────────────────────────*/

QVariant GroupModel::dataForCharacter(const SharedGroupChar &pCharacter,
                                      const ColumnTypeEnum column,
                                      const int role) const
{
    const CGroupChar &character = deref(pCharacter);

    switch (role) {
    /* display / icons ---------------------------------------------------- */
    case Qt::DecorationRole:
    case Qt::DisplayRole:
        return makeDisplayRole(character, column, m_tokenManager);

    /* tooltips ----------------------------------------------------------- */
    case Qt::ToolTipRole:
        return makeTooltipRole(character,
                               column,
                               (column == ColumnTypeEnum::HP_PERCENT
                                || column == ColumnTypeEnum::MANA_PERCENT
                                || column == ColumnTypeEnum::MOVES_PERCENT));

    /* colours & alignment ------------------------------------------------ */
    case Qt::BackgroundRole:
        return character.getColor();

    case Qt::ForegroundRole:
        return mmqt::textColor(character.getColor());

    case Qt::TextAlignmentRole:
        if (column != ColumnTypeEnum::NAME && column != ColumnTypeEnum::ROOM_NAME) {
            // QVariant(AlignmentFlag) ctor doesn’t exist;
            return static_cast<int>(Qt::AlignCenter);
        }
        break;

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

    if (index.row() < 0 || index.row() >= static_cast<int>(m_characters.size())) {
        return QVariant();
    }

    const SharedGroupChar &character = m_characters.at(static_cast<size_t>(index.row()));
    const ColumnTypeEnum column = static_cast<ColumnTypeEnum>(index.column());

    return dataForCharacter(character, column, role);
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
    , m_model(this)
{
    if (m_group) {
        m_model.setCharacters(m_group->selectAll());
    } else {
        m_model.setCharacters({});
    }
    m_model.setTokenManager(&tokenManager());

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_table = new QTableView(this);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_proxyModel = new GroupProxyModel(this);
    m_proxyModel->setSourceModel(&m_model);
    m_table->setModel(m_proxyModel);

    m_table->setDragEnabled(true);
    m_table->setAcceptDrops(true);
    m_table->setDragDropMode(QAbstractItemView::InternalMove);
    m_table->setDefaultDropAction(Qt::MoveAction);
    m_table->setDropIndicatorShown(true);

    m_table->setItemDelegate(new GroupDelegate(this));
    layout->addWidget(m_table);

    // Minimize row height
    const int icon = getConfig().groupManager.tokenIconSize;
    const int row = std::max(icon, m_table->fontMetrics().height() + 4);

    m_table->verticalHeader()->setDefaultSectionSize(row);
    m_table->setIconSize(QSize(icon, icon));
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

    // ── Set-icon action ───────────────────────────────────────────
    m_setIcon = new QAction(QIcon(":/icons/group-set-icon.png"), tr("Set &Icon…"), this);

    connect(m_setIcon, &QAction::triggered, this, [this]() {
        if (!selectedCharacter) // safety
            return;

        // 1. Character name (key)
        const QString charName = selectedCharacter->getDisplayName().trimmed();

        // 2. Tokens folder  (=  <resourcesDirectory>/tokens )
        const QString tokensDir = QDir(getConfig().canvas.resourcesDirectory).filePath("tokens");

        if (!QDir(tokensDir).exists()) {
            QMessageBox::information(this,
                                     tr("Tokens folder not found"),
                                     tr("No 'tokens' folder was found at:\n%1\n\n"
                                        "Create a folder named 'tokens' inside that directory, "
                                        "put your images there, then restart MMapper.")
                                         .arg(tokensDir));
            return; // abort setting an icon
        }

        const QString file = QFileDialog::getOpenFileName(this,
                                                          tr("Choose icon for %1").arg(charName),
                                                          tokensDir,
                                                          tr("Images (*.png *.jpg *.bmp *.svg)"));

        if (file.isEmpty())
            return; // user cancelled

        // 3. store only the basename (without path / extension)
        const QString base = QFileInfo(file).completeBaseName();
        setConfig().groupManager.tokenOverrides[charName] = base;

        // 4. immediately refresh this widget
        slot_updateLabels();
        emit sig_characterUpdated(selectedCharacter);
    });

    m_useDefaultIcon = new QAction(QIcon(":/icons/group-clear-icon.png"),
                                   tr("&Use default icon"),
                                   this);

    connect(m_useDefaultIcon, &QAction::triggered, this, [this]() {
        if (!selectedCharacter)
            return;

        const QString charName = selectedCharacter->getDisplayName().trimmed();

        // store the sentinel so TokenManager shows char-room-sel.png
        setConfig().groupManager.tokenOverrides[charName] = kForceFallback;

        slot_updateLabels(); // live refresh
        emit sig_characterUpdated(selectedCharacter);
    });

    connect(m_table, &QAbstractItemView::clicked, this, &GroupWidget::showContextMenu);

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
}

GroupWidget::~GroupWidget()
{
    delete m_table;
    delete m_recolor;
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
        for (const auto &character : m_model.getCharacters()) {
            if (character && character->getMana() > 0) {
                return true;
            }
        }
        return false;
    };
    const bool hide_mana = !one_character_had_mana();
    m_table->setColumnHidden(static_cast<int>(ColumnTypeEnum::MANA), hide_mana);
    m_table->setColumnHidden(static_cast<int>(ColumnTypeEnum::MANA_PERCENT), hide_mana);

    const bool hide_tokens = !getConfig().groupManager.showTokens;
    m_table->setColumnHidden(static_cast<int>(ColumnTypeEnum::CHARACTER_TOKEN), hide_tokens);

    // Apply current icon-size preference every time settings change
    {
        const int icon = getConfig().groupManager.tokenIconSize;
        m_table->setIconSize(QSize(icon, icon));

        QFontMetrics fm = m_table->fontMetrics();
        int row = std::max(icon, fm.height() + 4);
        m_table->verticalHeader()->setDefaultSectionSize(row);
    }
}

void GroupWidget::slot_onCharacterAdded(SharedGroupChar character)
{
    assert(character);
    m_model.insertCharacter(character);
    updateColumnVisibility();
}

void GroupWidget::slot_onCharacterRemoved(const GroupId characterId)
{
    assert(characterId != INVALID_GROUPID);
    m_model.removeCharacterById(characterId);
    updateColumnVisibility();
}

void GroupWidget::slot_onCharacterUpdated(SharedGroupChar character)
{
    assert(character);
    m_model.updateCharacter(character);
    updateColumnVisibility();
}

void GroupWidget::slot_onGroupReset(const GroupVector &newCharacterList)
{
    m_model.setCharacters(newCharacterList);
    updateColumnVisibility();
}

void GroupWidget::slot_updateLabels()
{
    m_model.resetModel(); // This re-fetches characters and refreshes the table
}

// ───────────────────────── context-menu helpers ─────────────────────────
void GroupWidget::showContextMenu(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid())
        return;

    QModelIndex src = m_proxyModel->mapToSource(proxyIndex);
    if (!src.isValid())
        return;

    selectedCharacter = m_model.getCharacter(src.row());
    if (!selectedCharacter)
        return;

    buildAndExecMenu();
}

void GroupWidget::buildAndExecMenu()
{
    const CGroupChar &c = deref(selectedCharacter);

    m_center->setText(QString("&Center on %1").arg(c.getName().toQString()));
    m_center->setDisabled(!c.isYou() && c.getServerId() == INVALID_SERVER_ROOMID);

    m_recolor->setText(QString("&Recolor %1").arg(c.getName().toQString()));
    m_setIcon->setText(QString("&Set icon for %1…").arg(c.getName().toQString()));
    m_useDefaultIcon->setText(QString("&Use default icon for %1").arg(c.getName().toQString()));

    QMenu menu(tr("Context menu"), this);
    menu.addAction(m_center);
    menu.addAction(m_recolor);
    menu.addAction(m_setIcon);
    menu.addAction(m_useDefaultIcon);
    menu.exec(QCursor::pos());
}
// ─────────────────────────────────────────────────────────────────────────
