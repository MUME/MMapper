#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"
#include "mmapper2character.h"

#include <QAbstractTableModel>
#include <QColor>
#include <QSortFilterProxyModel>
#include <QString>
#include <QtCore>

class QObject;
class QPainter;
class QRect;

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

// NOTE: paint() is implemented in groupwidget.cpp, since it depends on the
// GroupImageCache/getImage() helpers that remain there for the QTableView-based
// GroupDelegate. GroupStateData itself is just a plain-data snapshot produced by
// GroupModel::data() for ColumnTypeEnum::STATE.
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
    NODISCARD int getWidth() const { return static_cast<int>(m_count) * m_height; }
};
Q_DECLARE_METATYPE(GroupStateData)

#define XFOREACH_COLUMNTYPE(X) \
    X(NAME, name, Name, "Name") \
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

// Role URL scheme for the "stateIcons" role (Role::StateIconsRole below):
//   image://groupicons/<std|inv>/<position|affect>/<lowercase-enum-name>
// <std|inv> selects the standard or inverted (white-icon) variant, mirroring
// GroupImageCache's invert condition: mmqt::textColor(charColor) == Qt::white.
// <lowercase-enum-name> is the same "lower_case" token used by
// Filenames.cpp's getFilenameSuffix() (e.g. "standing", "sanctuary").
class NODISCARD_QOBJECT GroupModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    // Custom roles, in addition to the standard Qt::DisplayRole/ToolTipRole
    // consumed by the QTableView-based GroupWidget. All roles are answered
    // from data() regardless of the QModelIndex's column.
    enum Role {
        NameRole = Qt::UserRole + 1,
        CharColorRole,
        TextColorRole,
        HpTextRole,
        ManaTextRole,
        MovesTextRole,
        HpRatioRole,
        ManaRatioRole,
        MovesRatioRole,
        HpLowRole,
        MovesLowRole,
        ManaHiddenRole,
        StateIconsRole,
        StateTipRole,
        HpTipRole,
        ManaTipRole,
        MovesTipRole,
        RoomNameRole,
        IsYouRole,
        IsNpcRole,
        CanCenterRole,
    };

private:
    GroupVector m_characters;
    bool m_mapLoaded = false;
    bool m_anyMana = false;

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
    void resetModel();

public:
    // True when at least one character has (or has had) mana; recomputed on
    // every character add/remove/update/reset.
    NODISCARD bool getAnyMana() const { return m_anyMana; }
    NODISCARD bool moveRow(int from, int to);

signals:
    void sig_anyManaChanged();

private:
    NODISCARD QVariant dataForCharacter(const SharedGroupChar &character,
                                        ColumnTypeEnum column,
                                        int role) const;
    NODISCARD QVariant roleDataForCharacter(const SharedGroupChar &character, int role) const;
    NODISCARD int findIndexById(GroupId charId) const;
    void updateAnyMana();

protected:
    NODISCARD int rowCount(const QModelIndex &parent) const override;
    NODISCARD int columnCount(const QModelIndex &parent) const override;

    NODISCARD QVariant data(const QModelIndex &index, int role) const override;
    NODISCARD QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    NODISCARD QHash<int, QByteArray> roleNames() const override;

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
