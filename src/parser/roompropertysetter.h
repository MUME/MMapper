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

#ifndef FLAGSETTER
#define FLAGSETTER

#include <QMap>

#include "component.h"
#include "roomrecipient.h"

class AbstractAction;
class Coordinate;
class RoomAdmin;

class RoomPropertySetterSlave : public RoomRecipient
{
private:
	AbstractAction * action;
public:
	RoomPropertySetterSlave(AbstractAction * in_action) : action(in_action) {}
	void receiveRoom(RoomAdmin *, const Room *);
	bool getResult() {return !action;}
};

class RoomPropertySetter : public Component
{
private:
	Q_OBJECT
	QMap<QByteArray, uint> propPositions;
	QMap<QByteArray, uint> fieldValues;

public:
	RoomPropertySetter();
	virtual Qt::ConnectionType requiredConnectionType(const QString &) {return Qt::DirectConnection;}
	
public slots:
	void parseProperty(const QByteArray &, const Coordinate &);
	
signals:
	void sendToUser(const QByteArray&);
	void lookingForRooms(RoomRecipient *,const Coordinate &);
};

#endif
