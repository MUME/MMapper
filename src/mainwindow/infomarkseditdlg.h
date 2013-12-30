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
#include "ui_infomarkseditdlg.h"
#include "infomark.h"

class MapData;
class MapCanvas;

class InfoMarksEditDlg : public QDialog, private Ui::InfoMarksEditDlg
{
    Q_OBJECT

signals:
	void mapChanged();
	void closeEventReceived();

public slots:
	void objectListCurrentIndexChanged(const QString&);
	void objectTypeCurrentIndexChanged(const QString&);
	 
	void objectNameTextChanged(QString);
	void objectTextChanged(QString);
	void x1ValueChanged(double);
	void y1ValueChanged(double);
	void x2ValueChanged(double);
	void y2ValueChanged(double);
	void layerValueChanged(int);
	void createClicked();
	void modifyClicked();
	void deleteClicked();	
	void onMoveNorthClicked();
	void onMoveSouthClicked();
	void onMoveEastClicked();
	void onMoveWestClicked();
	void onMoveUpClicked();
	void onMoveDownClicked();
	void onDeleteAllClicked();
	
	 
public:
    InfoMarksEditDlg(MapData* mapData, QWidget *parent = 0);
    ~InfoMarksEditDlg();
    
    void setPoints(double x1, double y1, double x2, double y2, int layer);
    
    void readSettings();
	void writeSettings();
	
protected:
	void closeEvent(QCloseEvent*);

    
private:

	MapData* m_mapData;

	double m_selX1, m_selY1, m_selX2, m_selY2;
	int m_selLayer;

	void connectAll();
	void disconnectAll();

	InfoMarkType getType();
	InfoMark* getInfoMark(QString name);
	InfoMark* getCurrentInfoMark();
	void setCurrentInfoMark(InfoMark* m);

	void updateMarkers();
	void updateDialog();
};


#endif
