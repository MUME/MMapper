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

#include <QColor>
#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>
#include <QtCore>

#include "groupselection.h"

class MapData;
class Mmapper2Group;
class QObject;
class QTableWidget;
class QTableWidgetItem;

class GroupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GroupWidget(Mmapper2Group *groupManager, MapData *md, QWidget *parent = nullptr);
    virtual ~GroupWidget();

    void setItemText(QTableWidgetItem *item, const QString &, const QColor &color);

public slots:
    void updateLabels();
    void messageBox(const QString &title, const QString &message);

private:
    QTableWidget *m_table = nullptr;
    Mmapper2Group *m_group = nullptr;
    MapData *m_map = nullptr;
    QHash<QString, QList<QTableWidgetItem *>> m_nameItemHash{};
};

#endif // GROUPWIDGET_H
