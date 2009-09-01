/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
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

#ifndef ROOMEDITATTRDLG_H
#define ROOMEDITATTRDLG_H

#include <QDialog>
#include "ui_roomeditattrdlg.h"

class QShortcut;
class MapData;
class MapCanvas;
class RoomSelection;
class Room;

class RoomEditAttrDlg : public QDialog, private Ui::RoomEditAttrDlg
{
    Q_OBJECT

signals:
	void mapChanged();

public slots:
    void setRoomSelection(const RoomSelection*, MapData*, MapCanvas*);   

	//selection page    
    void roomListCurrentIndexChanged(int);

	//attributes page
	void neutralRadioButtonToggled(bool);
	void goodRadioButtonToggled(bool);
	void evilRadioButtonToggled(bool);
	void alignUndefRadioButtonToggled(bool);
	
	void noPortRadioButtonToggled(bool);
	void portableRadioButtonToggled(bool);
	void portUndefRadioButtonToggled(bool);

	void noRideRadioButtonToggled(bool);
	void ridableRadioButtonToggled(bool);
	void rideUndefRadioButtonToggled(bool);
		
	void litRadioButtonToggled(bool);
	void darkRadioButtonToggled(bool);
	void lightUndefRadioButtonToggled(bool);
	
	void mobFlagsListItemChanged(QListWidgetItem*);
	void loadFlagsListItemChanged(QListWidgetItem*);
        
	void exitButtonToggled(bool);

	void exitFlagsListItemChanged(QListWidgetItem*);

	void doorNameLineEditTextChanged(QString);
	void doorFlagsListItemChanged(QListWidgetItem*);

  void toggleHiddenDoor();

	//terrain tab	
	void terrainToolButtonToggled(bool);
	
	//note tab
	void roomNoteChanged();
	
	//all tabs
	void closeClicked();
	    
public:
    RoomEditAttrDlg(QWidget *parent = 0);
    ~RoomEditAttrDlg();
    
    void readSettings();
	void writeSettings();
    
    
private:

	void connectAll();
	void disconnectAll();

	const Room* getSelectedRoom();	
	uint getSelectedExit();
	void updateDialog(const Room *r);
	
	QListWidgetItem* loadListItems[20];
	QListWidgetItem* mobListItems[20];
	
	QListWidgetItem* exitListItems[20];
	QListWidgetItem* doorListItems[20];

	const RoomSelection* 	m_roomSelection;
	MapData* 		m_mapData;
	MapCanvas* 		m_mapCanvas;
  QShortcut *m_hiddenShortcut;
};


#endif
