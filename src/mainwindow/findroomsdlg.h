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
#include "ui_findroomsdlg.h"
#include "abstractparser.h"

class MapData;
class MapCanvas;
class QShortcut;
class RoomSelection;

class FindRoomsDlg : public QDialog, private Ui::FindRoomsDlg
{
    Q_OBJECT

signals:
    //void lookingForRooms(RoomRecipient *,ParseEvent *);
    void center(qint32 x, qint32 y);
    void log( const QString &, const QString & );
    //void newRoomSelection(const RoomSelection*);

public slots:
    void closeEvent( QCloseEvent *event );

public:
    FindRoomsDlg(MapData *, QWidget *parent = 0);
    ~FindRoomsDlg();

private:
    MapData *m_mapData;
    QTreeWidgetItem *item{};
    QShortcut *m_showSelectedRoom;

    void adjustResultTable();

    static const QString nullString;
    const RoomSelection *m_roomSelection;

private slots:
    QString constructToolTip(const Room *);
    void on_lineEdit_textChanged();
    void findClicked();
    void enableFindButton(const QString &text);
    void itemDoubleClicked(QTreeWidgetItem *item);
    void showSelectedRoom();
};

#endif

