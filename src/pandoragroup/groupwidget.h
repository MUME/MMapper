#pragma once
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

#ifndef GROUPWIDGET_H
#define GROUPWIDGET_H

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

class GroupStateData
{
public:
    explicit GroupStateData() = default;
    explicit GroupStateData(const QColor &color,
                            const CharacterPosition position,
                            const CharacterAffects affects);

public:
    void paint(QPainter *painter, const QRect &rect);
    QSize sizeHint() const;

private:
    QColor color;
    CharacterPosition position;
    CharacterAffects affects;
    int imageCount;
};
Q_DECLARE_METATYPE(GroupStateData)

class GroupDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit GroupDelegate(QObject *parent = nullptr);
    ~GroupDelegate() override;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class GroupModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class ColumnType {
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

    explicit GroupModel(MapData *md, Mmapper2Group *group, QObject *parent = nullptr);

    void resetModel();
    QVariant dataForCharacter(CGroupChar *character, ColumnType column, int role) const;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &parent) const override;

    void setMapLoaded(const bool val) { m_mapLoaded = val; }

private:
    MapData *m_map = nullptr;
    Mmapper2Group *m_group = nullptr;
    bool m_mapLoaded = false;
};

class GroupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GroupWidget(Mmapper2Group *group, MapData *md, QWidget *parent = nullptr);
    virtual ~GroupWidget();

public slots:
    void updateLabels();
    void messageBox(const QString &title, const QString &message);
    void mapUnloaded() { m_model.setMapLoaded(false); }
    void mapLoaded() { m_model.setMapLoaded(true); }

signals:
    void kickCharacter(const QByteArray &);

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
    QByteArray selectedCharacter{};
};

#endif // GROUPWIDGET_H
