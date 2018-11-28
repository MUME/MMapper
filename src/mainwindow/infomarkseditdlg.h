#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef INFOMARKSEDITDLG_H
#define INFOMARKSEDITDLG_H

#include <QDialog>
#include <QString>
#include <QtCore>

#include "../mapdata/infomark.h"
#include "ui_infomarkseditdlg.h"

class Coordinate;
class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QWidget;
class InfoMarkSelection;

class InfoMarksEditDlg : public QDialog, private Ui::InfoMarksEditDlg
{
    Q_OBJECT

public:
    explicit InfoMarksEditDlg(QWidget *parent = nullptr);
    ~InfoMarksEditDlg() override;

    void setInfoMarkSelection(InfoMarkSelection *is, MapData *md, MapCanvas *mc);

signals:
    void mapChanged();

public slots:
    void objectListCurrentIndexChanged(const QString &);
    void objectTypeCurrentIndexChanged(const QString &);

    void createClicked();
    void modifyClicked();

private:
    InfoMarkSelection *m_selection = nullptr;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;

    void connectAll();
    void disconnectAll();

    void readSettings();
    void writeSettings();

    InfoMarkType getType();
    InfoMarkClass getClass();
    InfoMark *getInfoMark(const QString &name);
    InfoMark *getCurrentInfoMark();
    void setCurrentInfoMark(InfoMark *m);

    void updateMarkers();
    void updateDialog();
};

#endif
