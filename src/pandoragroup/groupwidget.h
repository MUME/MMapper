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
#include <QWidget>
#include <QtCore>

#include "groupselection.h"

class MapData;
class CGroupChar;
class Mmapper2Group;
class QObject;
class QTableView;

class GroupModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit GroupModel(MapData *md, QObject *parent = nullptr);

    void setGroupSelection(GroupSelection *const selection);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &parent = QModelIndex()) const override;

private:
    MapData *m_map = nullptr;
    std::vector<CGroupChar *> m_characters;
};

class GroupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GroupWidget(Mmapper2Group *groupManager, MapData *md, QWidget *parent = nullptr);
    virtual ~GroupWidget();

public slots:
    void updateLabels();
    void messageBox(const QString &title, const QString &message);

private:
    QTableView *m_table = nullptr;
    Mmapper2Group *m_group = nullptr;
    MapData *m_map = nullptr;
    GroupModel m_model;
};

#endif // GROUPWIDGET_H
