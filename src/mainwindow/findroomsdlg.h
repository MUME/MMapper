#pragma once
/************************************************************************
**
** Authors:   Kalev Lember <kalev@smartlink.ee>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QString>
#include <QtCore>
#include <QtGlobal>
#include <QtWidgets/QTreeWidgetItem>

#include "../mapdata/roomselection.h"
#include "../parser/abstractparser.h"
#include "ui_findroomsdlg.h" // auto-generated

class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QShortcut;
class QTreeWidgetItem;
class QWidget;
class Room;

class FindRoomsDlg : public QDialog, private Ui::FindRoomsDlg
{
    Q_OBJECT

signals:
    void center(qint32 x, qint32 y);
    void newRoomSelection(const SigRoomSelection &);
    void editSelection();
    void log(const QString &, const QString &);

public slots:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit FindRoomsDlg(MapData *, QWidget *parent = nullptr);
    ~FindRoomsDlg() override;

private:
    MapData *m_mapData = nullptr;
    QTreeWidgetItem *item = nullptr;
    QShortcut *m_showSelectedRoom = nullptr;

    void adjustResultTable();

    static const QString nullString;

private slots:
    QString constructToolTip(const Room *);
    void on_lineEdit_textChanged();
    void findClicked();
    void enableFindButton(const QString &text);
    void itemDoubleClicked(QTreeWidgetItem *inputItem);
    void showSelectedRoom();
};

#endif
