#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <vector>
#include <QAbstractTableModel>
#include <QString>
#include <QStyledItemDelegate>
#include <QWidget>
#include <QtCore>

#include "groupselection.h"
#include "mmapper2character.h"

class QAction;
class MapData;
class CGroupChar;
class Mmapper2Group;
class QObject;
class QTableView;

class NODISCARD GroupStateData final
{
public:
    GroupStateData() = default;
    explicit GroupStateData(const QColor &color,
                            CharacterPositionEnum position,
                            CharacterAffectFlags affects);

public:
    void paint(QPainter *painter, const QRect &rect);
    NODISCARD int getWidth() const { return count * height; }

private:
    QColor color;
    CharacterPositionEnum position = CharacterPositionEnum::UNDEFINED;
    CharacterAffectFlags affects;
    int count = 0;
    int height = 23;
};
Q_DECLARE_METATYPE(GroupStateData)

class GroupDelegate final : public QStyledItemDelegate
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

class GroupModel final : public QAbstractTableModel
{
    Q_OBJECT

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

    explicit GroupModel(MapData *md, Mmapper2Group *group, QObject *parent);

    void resetModel();
    NODISCARD QVariant dataForCharacter(const SharedGroupChar &character,
                                        ColumnTypeEnum column,
                                        int role) const;

    NODISCARD int rowCount(const QModelIndex &parent) const override;
    NODISCARD int columnCount(const QModelIndex &parent) const override;

    NODISCARD QVariant data(const QModelIndex &index, int role) const override;
    NODISCARD QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    NODISCARD Qt::ItemFlags flags(const QModelIndex &parent) const override;

    void setMapLoaded(const bool val) { m_mapLoaded = val; }

private:
    MapData *m_map = nullptr;
    Mmapper2Group *m_group = nullptr;
    bool m_mapLoaded = false;
};

class GroupWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit GroupWidget(Mmapper2Group *group, MapData *md, QWidget *parent);
    ~GroupWidget() final;

public slots:
    void slot_updateLabels();
    void slot_messageBox(const QString &title, const QString &message);
    void slot_mapUnloaded() { m_model.setMapLoaded(false); }
    void slot_mapLoaded() { m_model.setMapLoaded(true); }

signals:
    void sig_kickCharacter(const QByteArray &);
    void sig_center(glm::vec2);

private:
    void readSettings();
    void writeSettings();

private:
    QTableView *m_table = nullptr;
    Mmapper2Group *m_group = nullptr;
    MapData *m_map = nullptr;
    GroupModel m_model;

private:
    QAction *m_kick = nullptr;
    QByteArray selectedCharacter;
};
