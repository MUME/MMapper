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

#ifndef CONNECTIONSELECTION_H
#define CONNECTIONSELECTION_H

#include <QObject>
#include "roomrecipient.h"
#include "mmapper2exit.h"

class MapFrontend;
class Room;

class ConnectionSelection : public QObject, public RoomRecipient
{
    Q_OBJECT

public:
	
	struct ConnectionDescriptor
	{
		const Room* room;	
		ExitDirection direction;
	};

    ConnectionSelection(MapFrontend* , double mx, double my, int layer);
    ConnectionSelection();
    ~ConnectionSelection();
    
    void setFirst(MapFrontend* , double mx, double my, int layer);
	void setFirst(MapFrontend* mf, uint id, ExitDirection dir);
    void setSecond(MapFrontend* , double mx, double my, int layer);
	void setSecond(MapFrontend* mf, uint id, ExitDirection dir);
	
	void removeFirst();
	void removeSecond();
	
	ConnectionDescriptor getFirst();
	ConnectionDescriptor getSecond();

	bool isValid();
	bool isFirstValid() { if(m_connectionDescriptor[0].room == NULL) return false; else return true; };
	bool isSecondValid(){ if(m_connectionDescriptor[1].room == NULL) return false; else return true; };
	
	void receiveRoom(RoomAdmin * admin, const Room * aRoom);
    
public slots:

signals:

protected:
	
private:

	ExitDirection ComputeDirection(double mouseX, double mouseY);

	int GLtoMap(double arg);

	ConnectionDescriptor m_connectionDescriptor[2];

	bool m_first;
	RoomAdmin * m_admin;
};


#endif
