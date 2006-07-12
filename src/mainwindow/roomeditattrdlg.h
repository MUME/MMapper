/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef ROOMEDITATTRDLG_H
#define ROOMEDITATTRDLG_H

#include <QDialog>
#include "ui_roomeditattrdlg.h"

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
};


#endif
