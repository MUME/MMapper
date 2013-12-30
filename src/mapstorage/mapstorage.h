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

#ifndef MAPSTORAGE_H
#define MAPSTORAGE_H

#include "mapdata.h"
#include "abstractmapstorage.h"
#include "oldroom.h"
#include "roomfactory.h"

class MapStorage : public AbstractMapStorage {

    Q_OBJECT

public:
    MapStorage(MapData&, const QString&, QFile*);
    MapStorage(MapData&, const QString&);
    bool mergeData();
    
private:
    virtual bool canLoad() {return TRUE;};
    virtual bool canSave() {return TRUE;};

    virtual void newData ();
    virtual bool loadData();
    virtual bool saveData( bool baseMapOnly );

    RoomFactory factory;
    Room * loadRoom(QDataStream & stream, qint32 version);
    void loadExits(Room * room, QDataStream & stream, qint32 version);
    Room * loadOldRoom(QDataStream & stream, ConnectionList & connectionList);
    void loadOldConnection(Connection *, QDataStream & stream, RoomList & roomList);
    void loadMark(InfoMark * mark, QDataStream & stream, qint32 version);
    void saveMark(InfoMark * mark, QDataStream & stream);
    void translateOldConnection(Connection *);
    void saveRoom(const Room * room, QDataStream & stream);
    void saveExits(const Room * room, QDataStream & stream);
    
    uint baseId;
    Coordinate basePosition;
};

class MapFrontendBlocker {
  public:
    MapFrontendBlocker(MapFrontend & in_data) : data(in_data) {data.block();}
    ~MapFrontendBlocker() {data.unblock();}
  private:
    MapFrontend & data;
};


#endif
