#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"
#include "mmapper2character.h"

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

class NODISCARD_QOBJECT GroupModel final : public QAbstractTableModel
{
    Q_OBJECT

private:
    GroupVector m_characters;
    bool m_mapLoaded = false;

public:
    enum class NODISCARD ColumnTypeEnum {
        NAME = 0,
        HP_PERCENT,
        MANA_PERCENT,
        MOVES_PERCENT,
        HP,
        MANA,
        MOVES,
        STATE,
        ROOM_NAME
    };

    explicit GroupModel(QObject *parent = nullptr);

    NODISCARD const GroupVector &getCharacters() const { return m_characters; }
    void setCharacters(const GroupVector &newChars);
    void resetModel();
    NODISCARD QVariant dataForCharacter(const SharedGroupChar &character,
                                        ColumnTypeEnum column,
                                        int role) const;
    NODISCARD SharedGroupChar getCharacter(int row) const;

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

    void setMapLoaded(const bool val) { m_mapLoaded = val; }
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

private:
    QAction *m_center = nullptr;
    QAction *m_recolor = nullptr;
    SharedGroupChar selectedCharacter;

public:
    explicit GroupWidget(Mmapper2Group *group, MapData *md, QWidget *parent);
    ~GroupWidget() final;

protected:
    NODISCARD QSize sizeHint() const override;

signals:
    void sig_kickCharacter(const QString &);
    void sig_center(glm::vec2);

public slots:
    void slot_updateLabels();
    void slot_mapUnloaded() { m_model.setMapLoaded(false); }
    void slot_mapLoaded() { m_model.setMapLoaded(true); }
};
