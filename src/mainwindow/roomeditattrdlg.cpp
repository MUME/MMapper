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

#include "roomeditattrdlg.h"
#include "mapdata.h"
#include "mapcanvas.h"
#include "roomselection.h"
#include "room.h"
#include "mmapper2room.h"
#include "customaction.h"
#include "defs.h"

#include <QShortcut>
#include <QSettings>

#include <math.h>

RoomEditAttrDlg::RoomEditAttrDlg(QWidget *parent)
        : QDialog(parent)
{
    setupUi(this);
	roomDescriptionTextEdit->setLineWrapMode(QTextEdit::NoWrap);
		
	mobListItems[0] = (QListWidgetItem*) new QListWidgetItem("Rent place");
	mobListItems[1] = (QListWidgetItem*) new QListWidgetItem("Generic shop");
	mobListItems[2] = (QListWidgetItem*) new QListWidgetItem("Weapon shop");
	mobListItems[3] = (QListWidgetItem*) new QListWidgetItem("Armour shop");
	mobListItems[4] = (QListWidgetItem*) new QListWidgetItem("Food shop");
	mobListItems[5] = (QListWidgetItem*) new QListWidgetItem("Pet shop");
	mobListItems[6] = (QListWidgetItem*) new QListWidgetItem("Generic guild");
	mobListItems[7] = (QListWidgetItem*) new QListWidgetItem("Scout guild");
	mobListItems[8] = (QListWidgetItem*) new QListWidgetItem("Mage guild");
	mobListItems[9] = (QListWidgetItem*) new QListWidgetItem("Cleric guild");
	mobListItems[10] = (QListWidgetItem*) new QListWidgetItem("Warrior guild");
	mobListItems[11] = (QListWidgetItem*) new QListWidgetItem("Ranger guild");
	mobListItems[12] = (QListWidgetItem*) new QListWidgetItem("SMOB");
	mobListItems[13] = (QListWidgetItem*) new QListWidgetItem("Quest mob");
	mobListItems[14] = (QListWidgetItem*) new QListWidgetItem("Any mob"); 
	mobListItems[15] = NULL;

	mobFlagsListWidget->clear();
	for (int i=0; i<15; i++)
	{	
		mobListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
		mobFlagsListWidget->addItem(mobListItems[i]);
	}

	loadListItems[0] = (QListWidgetItem*) new QListWidgetItem("Treasure");
	loadListItems[1] = (QListWidgetItem*) new QListWidgetItem("Equipment");
	loadListItems[2] = (QListWidgetItem*) new QListWidgetItem("Weapon");
	loadListItems[3] = (QListWidgetItem*) new QListWidgetItem("Water");
	loadListItems[4] = (QListWidgetItem*) new QListWidgetItem("Food");
	loadListItems[5] = (QListWidgetItem*) new QListWidgetItem("Herb");
	loadListItems[6] = (QListWidgetItem*) new QListWidgetItem("Key");
	loadListItems[7] = (QListWidgetItem*) new QListWidgetItem("Mule");
	loadListItems[8] = (QListWidgetItem*) new QListWidgetItem("Horse");
	loadListItems[9] = (QListWidgetItem*) new QListWidgetItem("Pack horse");
	loadListItems[10] = (QListWidgetItem*) new QListWidgetItem("Trained horse");
	loadListItems[11] = (QListWidgetItem*) new QListWidgetItem("Rohirrim");
	loadListItems[12] = (QListWidgetItem*) new QListWidgetItem("Warg"); 
	loadListItems[13] = (QListWidgetItem*) new QListWidgetItem("Boat"); 
	loadListItems[14] = (QListWidgetItem*) new QListWidgetItem("Attention");
	loadListItems[15] = (QListWidgetItem*) new QListWidgetItem("Tower"); 
	loadListItems[16] = NULL;
		
	loadFlagsListWidget->clear();
	for (int i=0; i<16; i++)
	{
		loadListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
		loadFlagsListWidget->addItem(loadListItems[i]);
	}

	exitListItems[0] = (QListWidgetItem*) new QListWidgetItem("Exit");
	exitListItems[1] = (QListWidgetItem*) new QListWidgetItem("Door");
	exitListItems[2] = (QListWidgetItem*) new QListWidgetItem("Road");
	exitListItems[3] = (QListWidgetItem*) new QListWidgetItem("Climb");
	exitListItems[4] = (QListWidgetItem*) new QListWidgetItem("Random");
	exitListItems[5] = (QListWidgetItem*) new QListWidgetItem("Special");
	exitListItems[6] = (QListWidgetItem*) new QListWidgetItem("No match");
	exitListItems[7] = NULL;

	exitFlagsListWidget->clear();
	for (int i=0; i<7; i++)
	{	
		exitListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		exitFlagsListWidget->addItem(exitListItems[i]);
	}
	
	doorListItems[0] = (QListWidgetItem*) new QListWidgetItem("Hidden");
	doorListItems[1] = (QListWidgetItem*) new QListWidgetItem("Need key");
	doorListItems[2] = (QListWidgetItem*) new QListWidgetItem("No block");
	doorListItems[3] = (QListWidgetItem*) new QListWidgetItem("No break");
	doorListItems[4] = (QListWidgetItem*) new QListWidgetItem("No pick");
	doorListItems[5] = (QListWidgetItem*) new QListWidgetItem("Delayed");
	doorListItems[6] = NULL;

	doorFlagsListWidget->clear();
	for (int i=0; i<6; i++)
	{
		doorListItems[i]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		doorFlagsListWidget->addItem(doorListItems[i]);
	}

  m_hiddenShortcut = new QShortcut( QKeySequence( tr( "Ctrl+H", "Room edit > hidden flag" ) ), this );

	updatedLabel->setText("Room has not been online updated yet!!!");
		
	readSettings();	
}

RoomEditAttrDlg::~RoomEditAttrDlg()
{
	writeSettings();
}

void RoomEditAttrDlg::readSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("RoomEditAttrDlg");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    settings.endGroup();
    move(pos);
}

void RoomEditAttrDlg::writeSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("RoomEditAttrDlg");
    settings.setValue("pos", pos());
    settings.endGroup();
}

void RoomEditAttrDlg::connectAll()
{
    connect(neutralRadioButton, SIGNAL(toggled(bool)), this, SLOT(neutralRadioButtonToggled(bool)));	
    connect(goodRadioButton, SIGNAL(toggled(bool)), this, SLOT(goodRadioButtonToggled(bool)));	
    connect(evilRadioButton, SIGNAL(toggled(bool)), this, SLOT(evilRadioButtonToggled(bool)));	
    connect(alignUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(alignUndefRadioButtonToggled(bool)));	

    connect(noPortRadioButton, SIGNAL(toggled(bool)), this, SLOT(noPortRadioButtonToggled(bool)));	
    connect(portableRadioButton, SIGNAL(toggled(bool)), this, SLOT(portableRadioButtonToggled(bool)));	
    connect(portUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(portUndefRadioButtonToggled(bool)));	
	
    connect(noRideRadioButton, SIGNAL(toggled(bool)), this, SLOT(noRideRadioButtonToggled(bool)));	
    connect(ridableRadioButton, SIGNAL(toggled(bool)), this, SLOT(ridableRadioButtonToggled(bool)));	
    connect(rideUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(rideUndefRadioButtonToggled(bool)));	

    connect(litRadioButton, SIGNAL(toggled(bool)), this, SLOT(litRadioButtonToggled(bool)));	
    connect(darkRadioButton, SIGNAL(toggled(bool)), this, SLOT(darkRadioButtonToggled(bool)));	
    connect(lightUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(lightUndefRadioButtonToggled(bool)));	
	
	connect(mobFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(mobFlagsListItemChanged(QListWidgetItem*)));
	connect(loadFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(loadFlagsListItemChanged(QListWidgetItem*)));

    connect(exitNButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    connect(exitSButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    connect(exitEButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    connect(exitWButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    connect(exitUButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    connect(exitDButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	

	connect(exitFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(exitFlagsListItemChanged(QListWidgetItem*)));
	connect(doorFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(doorFlagsListItemChanged(QListWidgetItem*)));

	connect(doorNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(doorNameLineEditTextChanged(QString)));

	connect(toolButton00, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton01, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton02, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton03, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton04, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton05, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton06, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton07, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton08, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton09, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton10, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton11, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton12, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton13, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton14, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	connect(toolButton15, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));

    connect(roomNoteTextEdit, SIGNAL(textChanged()), this, SLOT(roomNoteChanged()));

    connect(m_hiddenShortcut, SIGNAL(activated()), this, SLOT(toggleHiddenDoor()));
}

void RoomEditAttrDlg::disconnectAll()
{
    disconnect(neutralRadioButton, SIGNAL(toggled(bool)), this, SLOT(neutralRadioButtonToggled(bool)));	
    disconnect(goodRadioButton, SIGNAL(toggled(bool)), this, SLOT(goodRadioButtonToggled(bool)));	
    disconnect(evilRadioButton, SIGNAL(toggled(bool)), this, SLOT(evilRadioButtonToggled(bool)));	
    disconnect(alignUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(alignUndefRadioButtonToggled(bool)));	

    disconnect(noPortRadioButton, SIGNAL(toggled(bool)), this, SLOT(noPortRadioButtonToggled(bool)));	
    disconnect(portableRadioButton, SIGNAL(toggled(bool)), this, SLOT(portableRadioButtonToggled(bool)));	
    disconnect(portUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(portUndefRadioButtonToggled(bool)));	
	
    disconnect(noRideRadioButton, SIGNAL(toggled(bool)), this, SLOT(noRideRadioButtonToggled(bool)));	
    disconnect(ridableRadioButton, SIGNAL(toggled(bool)), this, SLOT(ridableRadioButtonToggled(bool)));	
    disconnect(rideUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(rideUndefRadioButtonToggled(bool)));	

    disconnect(litRadioButton, SIGNAL(toggled(bool)), this, SLOT(litRadioButtonToggled(bool)));	
    disconnect(darkRadioButton, SIGNAL(toggled(bool)), this, SLOT(darkRadioButtonToggled(bool)));	
    disconnect(lightUndefRadioButton, SIGNAL(toggled(bool)), this, SLOT(lightUndefRadioButtonToggled(bool)));	
	
	disconnect(mobFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(mobFlagsListItemChanged(QListWidgetItem*)));
	disconnect(loadFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(loadFlagsListItemChanged(QListWidgetItem*)));

    disconnect(exitNButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    disconnect(exitSButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    disconnect(exitEButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    disconnect(exitWButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    disconnect(exitUButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	
    disconnect(exitDButton, SIGNAL(toggled(bool)), this, SLOT(exitButtonToggled(bool)));	

	disconnect(exitFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(exitFlagsListItemChanged(QListWidgetItem*)));
	disconnect(doorFlagsListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(doorFlagsListItemChanged(QListWidgetItem*)));

	disconnect(doorNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(doorNameLineEditTextChanged(QString)));

	disconnect(toolButton00, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton01, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton02, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton03, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton04, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton05, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton06, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton07, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton08, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton09, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton10, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton11, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton12, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton13, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton14, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));
	disconnect(toolButton15, SIGNAL(toggled(bool)), this, SLOT(terrainToolButtonToggled(bool)));

    disconnect(roomNoteTextEdit, SIGNAL(textChanged()), this, SLOT(roomNoteChanged()));	

    disconnect(m_hiddenShortcut, SIGNAL(activated()), this, SLOT(toggleHiddenDoor()));
}

const Room* RoomEditAttrDlg::getSelectedRoom()
{
	if ( m_roomSelection->size() == 0 ) return NULL;
	if ( m_roomSelection->size() == 1 ) 
		return (m_roomSelection->values().front());
	else	
		return (m_roomSelection->value((roomListComboBox->itemData(roomListComboBox->currentIndex()).toInt())));
}

uint RoomEditAttrDlg::getSelectedExit()
{
	if (exitNButton->isChecked()) return 0;
	if (exitSButton->isChecked()) return 1;
	if (exitEButton->isChecked()) return 2;
	if (exitWButton->isChecked()) return 3;
	if (exitUButton->isChecked()) return 4;
	if (exitDButton->isChecked()) return 5;
	return 6;
}

void RoomEditAttrDlg::roomListCurrentIndexChanged(int)
{
	disconnectAll();
	alignUndefRadioButton->setChecked(TRUE);
	portUndefRadioButton->setChecked(TRUE);
	lightUndefRadioButton->setChecked(TRUE);
	connectAll();

	updateDialog( getSelectedRoom() );
}

void RoomEditAttrDlg::setRoomSelection(const RoomSelection* rs, MapData* md, MapCanvas* mc)
{
	m_roomSelection = rs; 
	m_mapData=md;
	m_mapCanvas=mc;
	
	roomListComboBox->clear();
	
    if (rs->size() > 1) 
    {
    	tabWidget->setCurrentWidget(selectionTab);
    	roomListComboBox->addItem("All", 0);
    	updateDialog(NULL);    	    	
    	
		disconnectAll();
		alignUndefRadioButton->setChecked(TRUE);
		portUndefRadioButton->setChecked(TRUE);
		lightUndefRadioButton->setChecked(TRUE);
		connectAll();
    }
    else if (rs->size() == 1) 
    {
    	tabWidget->setCurrentWidget(attributesTab);
    	updateDialog(m_roomSelection->values().front());    	
    }
    
    
   RoomSelection::const_iterator i = m_roomSelection->constBegin();
    while (i != m_roomSelection->constEnd()) {
    	roomListComboBox->addItem(getName(i.value()), i.value()->getId());
        ++i;
    }
        
    connect(roomListComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(roomListCurrentIndexChanged(int)) );	
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));	
    connect(this, SIGNAL(mapChanged()), m_mapCanvas, SLOT(update()));	
}


void RoomEditAttrDlg::updateDialog(const Room *r)
{
	disconnectAll();
	
	if (r == NULL) 
	{
		roomDescriptionTextEdit->clear();
		roomDescriptionTextEdit->setEnabled(false);

		updatedLabel->setText("");

		roomNoteTextEdit->clear();
		roomNoteTextEdit->setEnabled(false);

		terrainLabel->setPixmap(QPixmap(QString(":/pixmaps/terrain%1.png").arg(0)));
		
		exitsFrame->setEnabled(false);                                

		int index = 0;
		while(loadListItems[index]!=NULL)
		{
			loadListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
			loadListItems[index]->setCheckState(Qt::PartiallyChecked);
			index++;		
		}
	
		index = 0;
		while(mobListItems[index]!=NULL)
		{
			mobListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate);
			mobListItems[index]->setCheckState(Qt::PartiallyChecked);		
			index++;		
		}
	}
	else
	{
		roomDescriptionTextEdit->clear();
		roomDescriptionTextEdit->setEnabled(true);

		if (r->isUpToDate()) 
			updatedLabel->setText("Room has been online updated.");
		else	
			updatedLabel->setText("Room has not been online updated yet!!!");

		exitsFrame->setEnabled(true);          
		
		const Exit &e = r->exit(getSelectedExit());
		ExitFlags ef = getFlags(e);                      
				
		if (ISSET(ef, EF_EXIT))	
			exitListItems[0]->setCheckState(Qt::Checked);
		else
			exitListItems[0]->setCheckState(Qt::Unchecked);
		
		if (ISSET(ef, EF_DOOR))
		{
			doorNameLineEdit->setEnabled(true);
			doorFlagsListWidget->setEnabled(true);	
						
			exitListItems[1]->setCheckState(Qt::Checked);			
			doorNameLineEdit->setText(getDoorName(e));		
				
			DoorFlags df = getDoorFlags(e);
			
			if (ISSET(df, DF_HIDDEN))	
				doorListItems[0]->setCheckState(Qt::Checked);
			else
				doorListItems[0]->setCheckState(Qt::Unchecked);
				
			if (ISSET(df, DF_NEEDKEY))	
				doorListItems[1]->setCheckState(Qt::Checked);
			else
				doorListItems[1]->setCheckState(Qt::Unchecked);
	
			if (ISSET(df, DF_NOBLOCK))	
				doorListItems[2]->setCheckState(Qt::Checked);
			else
				doorListItems[2]->setCheckState(Qt::Unchecked);
	
			if (ISSET(df, DF_NOBREAK))	
				doorListItems[3]->setCheckState(Qt::Checked);
			else
				doorListItems[3]->setCheckState(Qt::Unchecked);
	
			if (ISSET(df, DF_NOPICK))	
				doorListItems[4]->setCheckState(Qt::Checked);
			else
				doorListItems[4]->setCheckState(Qt::Unchecked);
	
			if (ISSET(df, DF_DELAYED))	
				doorListItems[5]->setCheckState(Qt::Checked);
			else
				doorListItems[5]->setCheckState(Qt::Unchecked);
		}
		else
		{
			doorNameLineEdit->clear();
			doorNameLineEdit->setEnabled(false);
			doorFlagsListWidget->setEnabled(false);				

			exitListItems[1]->setCheckState(Qt::Unchecked);
		}

		if (ISSET(ef, EF_ROAD))	
			exitListItems[2]->setCheckState(Qt::Checked);
		else
			exitListItems[2]->setCheckState(Qt::Unchecked);

		if (ISSET(ef, EF_CLIMB))	
			exitListItems[3]->setCheckState(Qt::Checked);
		else
			exitListItems[3]->setCheckState(Qt::Unchecked);
	
		if (ISSET(ef, EF_RANDOM))	
			exitListItems[4]->setCheckState(Qt::Checked);
		else
			exitListItems[4]->setCheckState(Qt::Unchecked);
	
		if (ISSET(ef, EF_SPECIAL))	
			exitListItems[5]->setCheckState(Qt::Checked);
		else
			exitListItems[5]->setCheckState(Qt::Unchecked);

		if (ISSET(ef, EF_NO_MATCH))	
			exitListItems[6]->setCheckState(Qt::Checked);
		else
			exitListItems[6]->setCheckState(Qt::Unchecked);

		roomNoteTextEdit->clear();
		roomNoteTextEdit->setEnabled(false);


		int index = 0;
		while(loadListItems[index]!=NULL)
		{
			loadListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			index++;		
		}
	
		index = 0;
		while(mobListItems[index]!=NULL)
		{
			mobListItems[index]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			index++;		
		}

		
		if (ISSET(getMobFlags(r),RMF_RENT)) 
			mobListItems[0]->setCheckState(Qt::Checked);
		else 
			mobListItems[0]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_SHOP))	
			mobListItems[1]->setCheckState(Qt::Checked);
		else
			mobListItems[1]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_WEAPONSHOP))	
			mobListItems[2]->setCheckState(Qt::Checked);
		else
			mobListItems[2]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_ARMOURSHOP))	
			mobListItems[3]->setCheckState(Qt::Checked);
		else
			mobListItems[3]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_FOODSHOP))	
			mobListItems[4]->setCheckState(Qt::Checked);
		else
			mobListItems[4]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_PETSHOP))	
			mobListItems[5]->setCheckState(Qt::Checked);
		else
			mobListItems[5]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_GUILD))	
			mobListItems[6]->setCheckState(Qt::Checked);
		else
			mobListItems[6]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_SCOUTGUILD))	
			mobListItems[7]->setCheckState(Qt::Checked);
		else
			mobListItems[7]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_MAGEGUILD))	
			mobListItems[8]->setCheckState(Qt::Checked);
		else
			mobListItems[8]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_CLERICGUILD))	
			mobListItems[9]->setCheckState(Qt::Checked);
		else
			mobListItems[9]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_WARRIORGUILD))	
			mobListItems[10]->setCheckState(Qt::Checked);
		else
			mobListItems[10]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_RANGERGUILD))	
			mobListItems[11]->setCheckState(Qt::Checked);
		else
			mobListItems[11]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_SMOB))	
			mobListItems[12]->setCheckState(Qt::Checked);
		else
			mobListItems[12]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_QUEST))	
			mobListItems[13]->setCheckState(Qt::Checked);
		else
			mobListItems[13]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getMobFlags(r),RMF_ANY))	
			mobListItems[14]->setCheckState(Qt::Checked);
		else
			mobListItems[14]->setCheckState(Qt::Unchecked);
	
	
		if (ISSET(getLoadFlags(r),RLF_TREASURE))	
			loadListItems[0]->setCheckState(Qt::Checked);
		else
			loadListItems[0]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_ARMOUR))	
			loadListItems[1]->setCheckState(Qt::Checked);
		else
			loadListItems[1]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_WEAPON))	
			loadListItems[2]->setCheckState(Qt::Checked);
		else
			loadListItems[2]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_WATER))	
			loadListItems[3]->setCheckState(Qt::Checked);
		else
			loadListItems[3]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_FOOD))	
			loadListItems[4]->setCheckState(Qt::Checked);
		else
			loadListItems[4]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_HERB))	
			loadListItems[5]->setCheckState(Qt::Checked);
		else
			loadListItems[5]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_KEY))	
			loadListItems[6]->setCheckState(Qt::Checked);
		else
			loadListItems[6]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_MULE))	
			loadListItems[7]->setCheckState(Qt::Checked);
		else
			loadListItems[7]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_HORSE))	
			loadListItems[8]->setCheckState(Qt::Checked);
		else
			loadListItems[8]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_PACKHORSE))	
			loadListItems[9]->setCheckState(Qt::Checked);
		else
			loadListItems[9]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_TRAINEDHORSE))	
			loadListItems[10]->setCheckState(Qt::Checked);
		else
			loadListItems[10]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_ROHIRRIM))	
			loadListItems[11]->setCheckState(Qt::Checked);
		else
			loadListItems[11]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_WARG))	
			loadListItems[12]->setCheckState(Qt::Checked);
		else
			loadListItems[12]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_BOAT))	
			loadListItems[13]->setCheckState(Qt::Checked);
		else
			loadListItems[13]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_ATTENTION))	
			loadListItems[14]->setCheckState(Qt::Checked);
		else
			loadListItems[14]->setCheckState(Qt::Unchecked);
	
		if (ISSET(getLoadFlags(r),RLF_TOWER))	
			loadListItems[15]->setCheckState(Qt::Checked);
		else
			loadListItems[15]->setCheckState(Qt::Unchecked);
	
	
		roomDescriptionTextEdit->setEnabled(true);
		roomNoteTextEdit->setEnabled(true);
				
		roomDescriptionTextEdit->clear();
		roomDescriptionTextEdit->setFontItalic(false);
		QString str = getDescription(r);
		str = str.left(str.length()-1);
		roomDescriptionTextEdit->append(str);	
		roomDescriptionTextEdit->setFontItalic(true);			
		roomDescriptionTextEdit->append(getDynamicDescription(r));	
		roomDescriptionTextEdit->scroll(-100,-100);
		
		roomNoteTextEdit->clear();
		roomNoteTextEdit->append(getNote(r));
		
		terrainLabel->setPixmap(QPixmap(QString(":/pixmaps/terrain%1.png").arg(getTerrainType(r))));
	
		switch (getAlignType(r))
		{
			case RAT_GOOD: 
				goodRadioButton->setChecked(TRUE); 
				break;
			case RAT_NEUTRAL: 
				neutralRadioButton->setChecked(TRUE); 
				break;
			case RAT_EVIL: 
				evilRadioButton->setChecked(TRUE); 
				break;
			case RAT_UNDEFINED:	
				alignUndefRadioButton->setChecked(TRUE);
				break;
		}	
	
		switch (getPortableType(r))
		{
			case RPT_PORTABLE: 
				portableRadioButton->setChecked(TRUE); 
				break;
			case RPT_NOTPORTABLE: 
				noPortRadioButton->setChecked(TRUE);
				break;
			case RPT_UNDEFINED:	
				portUndefRadioButton->setChecked(TRUE);
				break;
		}	
	
		switch (getRidableType(r))
		{
			case RRT_RIDABLE: 
				ridableRadioButton->setChecked(TRUE); 
				break;
			case RRT_NOTRIDABLE: 
				noRideRadioButton->setChecked(TRUE);
				break;
			case RRT_UNDEFINED:	
				rideUndefRadioButton->setChecked(TRUE);
				break;
		}	

		switch (getLightType(r))
		{
			case RLT_DARK: 
				darkRadioButton->setChecked(TRUE); 
				break;
			case RLT_LIT: 
				litRadioButton->setChecked(TRUE); 
				break;
			case RLT_UNDEFINED:	
				lightUndefRadioButton->setChecked(TRUE);
				break;
		}	
	}

	connectAll();
}




//attributes page
void RoomEditAttrDlg::exitButtonToggled(bool)
{
	updateDialog( getSelectedRoom() );	
}

void RoomEditAttrDlg::neutralRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_NEUTRAL, R_ALIGNTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_NEUTRAL, R_ALIGNTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::goodRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_GOOD, R_ALIGNTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_GOOD, R_ALIGNTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::evilRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_EVIL, R_ALIGNTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_EVIL, R_ALIGNTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::alignUndefRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RAT_UNDEFINED, R_ALIGNTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RAT_UNDEFINED, R_ALIGNTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

	
void RoomEditAttrDlg::noPortRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RPT_NOTPORTABLE, R_PORTABLETYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RPT_NOTPORTABLE, R_PORTABLETYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::portableRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RPT_PORTABLE, R_PORTABLETYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RPT_PORTABLE, R_PORTABLETYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::portUndefRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RPT_UNDEFINED, R_PORTABLETYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RPT_UNDEFINED, R_PORTABLETYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  									
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::noRideRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RRT_NOTRIDABLE, R_RIDABLETYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RRT_NOTRIDABLE, R_RIDABLETYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::ridableRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RRT_RIDABLE, R_RIDABLETYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RRT_RIDABLE, R_RIDABLETYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  										
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::rideUndefRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RRT_UNDEFINED, R_RIDABLETYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RRT_UNDEFINED, R_RIDABLETYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  									
		updateDialog( getSelectedRoom() );		
	}  										
}
		
void RoomEditAttrDlg::litRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RLT_LIT, R_LIGHTTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RLT_LIT, R_LIGHTTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  								
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::darkRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RLT_DARK, R_LIGHTTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RLT_DARK, R_LIGHTTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  								
		updateDialog( getSelectedRoom() );		
	}  										
}

void RoomEditAttrDlg::lightUndefRadioButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(RLT_UNDEFINED, R_LIGHTTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(RLT_UNDEFINED, R_LIGHTTYPE), m_roomSelection), m_roomSelection);		
	
		emit mapChanged();		  							
		updateDialog( getSelectedRoom() );		
	}  										
}

	
void RoomEditAttrDlg::mobFlagsListItemChanged(QListWidgetItem* item)
{
	int index = 0;
	while(item != mobListItems[index])
		index++;

	const Room* r = getSelectedRoom();

	switch (item->checkState())
	{
	case Qt::Unchecked:
		if (r)	
			m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags((uint)pow(2.0, index), R_MOBFLAGS, FMM_UNSET), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new ModifyRoomFlags((uint)pow(2.0, index), R_MOBFLAGS, FMM_UNSET), m_roomSelection), m_roomSelection);
		break;		
	case Qt::PartiallyChecked:
		break;
	case Qt::Checked:
		if (r)	
			m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags((uint)pow(2.0, index), R_MOBFLAGS, FMM_SET), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new ModifyRoomFlags((uint)pow(2.0, index), R_MOBFLAGS, FMM_SET), m_roomSelection), m_roomSelection);		
		break;		
	}
	
	emit mapChanged();		  						
}

void RoomEditAttrDlg::loadFlagsListItemChanged(QListWidgetItem* item)
{
	int index = 0;
	while(item != loadListItems[index])
		index++;

	const Room* r = getSelectedRoom();

	switch (item->checkState())
	{
	case Qt::Unchecked:
		if (r)	
			m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags((uint)pow(2.0, index), R_LOADFLAGS, FMM_UNSET), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new ModifyRoomFlags((uint)pow(2.0, index), R_LOADFLAGS, FMM_UNSET), m_roomSelection), m_roomSelection);
		break;		
	case Qt::PartiallyChecked:
		break;
	case Qt::Checked:
		if (r)	
			m_mapData->execute(new SingleRoomAction(new ModifyRoomFlags((uint)pow(2.0, index), R_LOADFLAGS, FMM_SET), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new ModifyRoomFlags((uint)pow(2.0, index), R_LOADFLAGS, FMM_SET), m_roomSelection), m_roomSelection);		
		break;		
	}
	
	emit mapChanged();		  							
}

void RoomEditAttrDlg::exitFlagsListItemChanged(QListWidgetItem* item)
{
	int index = 0;
	while(item != exitListItems[index])
		index++;

	const Room* r = getSelectedRoom();

	switch (item->checkState())
	{
	case Qt::Unchecked:
		if (r)	
			m_mapData->execute(new SingleRoomAction(new ModifyExitFlags((uint)pow(2.0, index), getSelectedExit(), E_FLAGS, FMM_UNSET), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new ModifyExitFlags((uint)pow(2.0, index), getSelectedExit(), E_FLAGS, FMM_UNSET), m_roomSelection), m_roomSelection);
		break;		
	case Qt::PartiallyChecked:
		break;
	case Qt::Checked:
		if (r)	
			m_mapData->execute(new SingleRoomAction(new ModifyExitFlags((uint)pow(2.0, index), getSelectedExit(), E_FLAGS, FMM_SET), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new ModifyExitFlags((uint)pow(2.0, index), getSelectedExit(), E_FLAGS, FMM_SET), m_roomSelection), m_roomSelection);		
		break;		
	}
	
	updateDialog( getSelectedRoom() );		
	emit mapChanged();		  					
}


void RoomEditAttrDlg::doorNameLineEditTextChanged(QString)
{
	const Room* r = getSelectedRoom();
	
	m_mapData->execute(new SingleRoomAction(new UpdateExitField(doorNameLineEdit->text(), getSelectedExit(), E_DOORNAME), r->getId()), m_roomSelection);
		
}

void RoomEditAttrDlg::doorFlagsListItemChanged(QListWidgetItem* item)
{
	int index = 0;
	while(item != doorListItems[index])
		index++;

	const Room* r = getSelectedRoom();

	switch (item->checkState())
	{
	case Qt::Unchecked:
		m_mapData->execute(new SingleRoomAction(new ModifyExitFlags((uint)pow(2.0, index), getSelectedExit(), E_DOORFLAGS, FMM_UNSET), r->getId()), m_roomSelection);
		break;		
	case Qt::PartiallyChecked:
		break;
	case Qt::Checked:
		m_mapData->execute(new SingleRoomAction(new ModifyExitFlags((uint)pow(2.0, index), getSelectedExit(), E_DOORFLAGS, FMM_SET), r->getId()), m_roomSelection);
		break;		
	}
			
	emit mapChanged();		  				
}

void RoomEditAttrDlg::toggleHiddenDoor()
{
  if ( doorFlagsListWidget->isEnabled() )  
    doorListItems[0]->setCheckState( doorListItems[0]->checkState() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked );
}

//terrain tab	
void RoomEditAttrDlg::terrainToolButtonToggled(bool val)
{
	if (val)
	{
		const Room* r = getSelectedRoom();
		
		uint index = 0;
		if (toolButton00->isChecked()) index = 0;
		if (toolButton01->isChecked()) index = 1;
		if (toolButton02->isChecked()) index = 2;
		if (toolButton03->isChecked()) index = 3;
		if (toolButton04->isChecked()) index = 4;
		if (toolButton05->isChecked()) index = 5;
		if (toolButton06->isChecked()) index = 6;
		if (toolButton07->isChecked()) index = 7;
		if (toolButton08->isChecked()) index = 8;
		if (toolButton09->isChecked()) index = 9;
		if (toolButton10->isChecked()) index = 10;
		if (toolButton11->isChecked()) index = 11;
		if (toolButton12->isChecked()) index = 12;
		if (toolButton13->isChecked()) index = 13;
		if (toolButton14->isChecked()) index = 14;
		if (toolButton15->isChecked()) index = 15;
		
		terrainLabel->setPixmap(QPixmap(QString(":/pixmaps/terrain%1.png").arg(index)));
	
		if (r)	
			m_mapData->execute(new SingleRoomAction(new UpdateRoomField(index, R_TERRAINTYPE), r->getId()), m_roomSelection);
		else
			m_mapData->execute(new GroupAction(new UpdateRoomField(index, R_TERRAINTYPE), m_roomSelection), m_roomSelection);
	
		emit mapChanged();		  				
	}
}
	
//note tab
void RoomEditAttrDlg::roomNoteChanged()
{
	const Room* r = getSelectedRoom();
	
	if (r)
		m_mapData->execute(new SingleRoomAction(new UpdateRoomField( roomNoteTextEdit->document()->toPlainText(), R_NOTE), r->getId()), m_roomSelection);	
	else
		m_mapData->execute(new GroupAction(new UpdateRoomField( roomNoteTextEdit->document()->toPlainText(), R_NOTE), m_roomSelection), m_roomSelection);	

	emit mapChanged();		  				
}

	
//all tabs
void RoomEditAttrDlg::closeClicked()
{
	accept();	
}

