#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"
#include "mmapper2character.h"
#include "tokenmanager.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStyledItemDelegate>
#include <QWidget>
#include <QtCore>

class QAction;
class MapData;
class Mmapper2Group;
class QObject;
class QTableView;

class NODISCARD_QOBJECT GroupProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit GroupProxyModel(QObject *parent = nullptr);
    ~GroupProxyModel() final;

    void refresh();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    SharedGroupChar getCharacterFromSource(const QModelIndex &source_index) const;
};

class NODISCARD GroupStateData final
{
private:
    QColor m_color;
    CharacterPositionEnum m_position = CharacterPositionEnum::UNDEFINED;
    CharacterAffectFlags m_affects;
    int m_count = 0;
    int m_height = 23;

public:
    GroupStateData() = default;
    explicit GroupStateData(const QColor &color,
                            CharacterPositionEnum position,
                            CharacterAffectFlags affects);

public:
    void paint(QPainter *pPainter, const QRect &rect);
    NODISCARD int getWidth() const { return m_count * m_height; }
};
Q_DECLARE_METATYPE(GroupStateData)

class NODISCARD_QOBJECT GroupDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit GroupDelegate(QObject *parent);
    ~GroupDelegate() final;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    NODISCARD QSize sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const override;
};

#define XFOREACH_COLUMNTYPE(X) \
    X(CHARACTER_TOKEN, character_token, CharacterToken, "Icon") \
    X(NAME, name, Name, "Name") \
    X(HP_PERCENT, hp_percent, HpPercent, "HP") \
    X(MANA_PERCENT, mana_percent, ManaPercent, "Mana") \
    X(MOVES_PERCENT, moves_percent, MovesPercent, "Moves") \
    X(HP, hp, Hp, "HP") \
    X(MANA, mana, Mana, "Mana") \
    X(MOVES, moves, Moves, "Moves") \
    X(STATE, state, State, "State") \
    X(ROOM_NAME, room_name, RoomName, "Room Name") \
    /* define column types above */

#define X_COUNT(UPPER_CASE, lower_case, CamelCase, friendly) +1
static constexpr const int GROUP_COLUMN_COUNT = XFOREACH_COLUMNTYPE(X_COUNT);
#undef X_COUNT

enum class NODISCARD ColumnTypeEnum {
#define X_DECL_COLUMNTYPE(UPPER_CASE, lower_case, CamelCase, friendly) UPPER_CASE,
    XFOREACH_COLUMNTYPE(X_DECL_COLUMNTYPE)
#undef X_DECL_COLUMNTYPE
};

static_assert(GROUP_COLUMN_COUNT == static_cast<int>(ColumnTypeEnum::ROOM_NAME) + 1, "# of columns");

class NODISCARD_QOBJECT GroupModel final : public QAbstractTableModel
{
    Q_OBJECT

private:
    GroupVector m_characters;
    bool m_mapLoaded = false;
    TokenManager *m_tokenManager = nullptr;

public:
    explicit GroupModel(QObject *parent = nullptr);

public:
    void setMapLoaded(const bool val) { m_mapLoaded = val; }

public:
    NODISCARD SharedGroupChar getCharacter(int row) const;
    NODISCARD const GroupVector &getCharacters() const { return m_characters; }
    void setCharacters(const GroupVector &newChars);
    void insertCharacter(const SharedGroupChar &newCharacter);
    void removeCharacterById(GroupId charId);
    void updateCharacter(const SharedGroupChar &updatedCharacter);
    void setTokenManager(TokenManager *manager) { m_tokenManager = manager; }
    void resetModel();

private:
    NODISCARD QVariant dataForCharacter(const SharedGroupChar &character,
                                        ColumnTypeEnum column,
                                        int role) const;
    NODISCARD int findIndexById(GroupId charId) const;

protected:
    NODISCARD int rowCount(const QModelIndex &parent) const override;
    NODISCARD int columnCount(const QModelIndex &parent) const override;

    NODISCARD QVariant data(const QModelIndex &index, int role) const override;
    NODISCARD QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Drag and drop overrides
    NODISCARD Qt::ItemFlags flags(const QModelIndex &index) const override;
    NODISCARD Qt::DropActions supportedDropActions() const override;
    NODISCARD QStringList mimeTypes() const override;
    NODISCARD QMimeData *mimeData(const QModelIndexList &indexes) const override;
    NODISCARD bool dropMimeData(const QMimeData *data,
                                Qt::DropAction action,
                                int row,
                                int column,
                                const QModelIndex &parent) override;
};

class NODISCARD_QOBJECT GroupWidget final : public QWidget
{
    Q_OBJECT

private:
    QTableView *m_table = nullptr;
    Mmapper2Group *m_group = nullptr;
    MapData *m_map = nullptr;
    GroupProxyModel *m_proxyModel = nullptr;
    GroupModel m_model;
    // TokenManager tokenManager;

    void updateColumnVisibility();
    void showContextMenu(const QModelIndex &proxyIndex);
    void buildAndExecMenu();

private:
    QAction *m_center = nullptr;
    QAction *m_recolor = nullptr;
    QAction *m_setIcon = nullptr;
    QAction *m_useDefaultIcon = nullptr;
    SharedGroupChar selectedCharacter;

public:
    explicit GroupWidget(Mmapper2Group *group, MapData *md, QWidget *parent);
    ~GroupWidget() final;

protected:
    NODISCARD QSize sizeHint() const override;

signals:
    void sig_kickCharacter(const QString &);
    void sig_center(glm::vec2);
    void sig_characterUpdated(SharedGroupChar character);

public slots:
    void slot_mapUnloaded() { m_model.setMapLoaded(false); }
    void slot_mapLoaded() { m_model.setMapLoaded(true); }

private slots:
    void slot_onCharacterAdded(SharedGroupChar character);
    void slot_onCharacterRemoved(GroupId characterId);
    void slot_onCharacterUpdated(SharedGroupChar character);
    void slot_onGroupReset(const GroupVector &newCharacterList);
    void slot_updateLabels();
};
