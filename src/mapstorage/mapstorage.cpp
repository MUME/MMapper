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

#include "mapstorage.h"
#include "mapdata.h"
#include "roomsaver.h"
#include "patterns.h"
#include "configuration.h"
#include "oldconnection.h"
#include "oldroom.h"
#include "mmapper2room.h"
#include "mmapper2exit.h"
#include "progresscounter.h"
#include "basemapsavefilter.h"
#include "infomark.h"
#include "qtiocompressor.h"
#include "olddoor.h"

#include <QFile>

#include <cassert>
#include <iostream>

using namespace std;

MapStorage::MapStorage(MapData& mapdata, const QString& filename, QFile* file) :
    AbstractMapStorage(mapdata, filename, file)
{}

MapStorage::MapStorage(MapData& mapdata, const QString& filename) :
    AbstractMapStorage(mapdata, filename)
{}

void MapStorage::newData()
{
  m_mapData.unsetDataChanged();

  m_mapData.setFileName(m_fileName);

  Coordinate pos;
  m_mapData.setPosition(pos);

  //clear previous map
  m_mapData.clear();
  emit onNewData();
}

Room * MapStorage::loadOldRoom(QDataStream & stream, ConnectionList & connectionList)
{
  Room * room = factory.createRoom();
  room->setPermanent();

  // set default values
  RoomTerrainType     terrainType = RTT_UNDEFINED;
  RoomPortableType    portableType = RPT_UNDEFINED;
  RoomRidableType     ridableType = RRT_UNDEFINED;
  RoomLightType       lightType = RLT_UNDEFINED;
  RoomAlignType       alignType = RAT_UNDEFINED;
  RoomMobFlags        mobFlags = 0;
  RoomLoadFlags       loadFlags = 0;
  QString vqstr;
  quint16 vquint16;
  quint32 vquint32;
  qint8 vqint8;

  stream >> vqstr;
  room->replace(R_NAME, vqstr);

  stream >> vqstr;
  QStringList list = vqstr.split('\n');
  QStringList::iterator i;
  bool readingStaticDescLines = true;
  QString staticRoomDesc;
  QString dynamicRoomDesc;

  qint32 lineCount = 0;

  for (i = list.begin(); i != list.end(); ++i)
  {
    if ( (*i) != "" )
    {
      if ((lineCount >= Config().m_minimumStaticLines) &&
          ((readingStaticDescLines == false) ||
           Patterns::matchDynamicDescriptionPatterns(*i)))
      {
        readingStaticDescLines = false;
        dynamicRoomDesc += (*i)+"\n";
        lineCount++;
      }
      else
      {
        staticRoomDesc += (*i)+"\n";
        lineCount++;
      }
    }
  }

  room->replace(R_DESC, staticRoomDesc);
  room->replace(R_DYNAMICDESC, dynamicRoomDesc);

  stream >> vquint16; //roomTerrain
  terrainType = (RoomTerrainType)(vquint16+1);

  stream >> vquint16; //roomMob
  switch ((int)vquint16)
  {
  case 1: SET(mobFlags, RMF_ANY); break;    //PEACEFULL
  case 2: SET(mobFlags, RMF_SMOB); break;   //AGGRESIVE
  case 3: SET(mobFlags, RMF_QUEST); break;  //QUEST
  case 4: SET(mobFlags, RMF_SHOP); break;   //SHOP
  case 5: SET(mobFlags, RMF_RENT); break;   //RENT
  case 6: SET(mobFlags, RMF_GUILD); break;  //GUILD
  default: break;
  }

  stream >> vquint16; //roomLoad
  switch ((int)vquint16)
  {
  case  1: SET(loadFlags, RLF_TREASURE); break;   //TREASURE
  case  2: SET(loadFlags, RLF_HERB); break;       //HERB
  case  3: SET(loadFlags, RLF_KEY); break;        //KEY
  case  4: SET(loadFlags, RLF_WATER); break;      //WATER
  case  5: SET(loadFlags, RLF_FOOD); break;       //FOOD
  case  6: SET(loadFlags, RLF_HORSE); break;      //HORSE
  case  7: SET(loadFlags, RLF_WARG); break;       //WARG
  case  8: SET(loadFlags, RLF_TOWER); break;   //TOWER
  case  9: SET(loadFlags, RLF_ATTENTION); break;  //ATTENTION
  case 10: SET(loadFlags, RLF_BOAT); break;       //BOAT
  default: break;
  }

  stream >> vquint16; //roomLocation { INDOOR, OUTSIDE }

  stream >> vquint16; //roomPortable { PORT, NOPORT }
  portableType = (RoomPortableType)(vquint16+1);
  stream >> vquint16; //roomLight { DARK, LIT }
  lightType = (RoomLightType)(vquint16+1);
  stream >> vquint16; //roomAlign { GOOD, NEUTRAL, EVIL }
  alignType = (RoomAlignType)(vquint16+1);

  stream >> vquint32; //roomFlags


  if (ISSET(vquint32, bit2))
    orExitFlags(room->exit(ED_NORTH), EF_EXIT);
  if (ISSET(vquint32, bit3))
    orExitFlags(room->exit(ED_SOUTH), EF_EXIT);
  if (ISSET(vquint32, bit4))
    orExitFlags(room->exit(ED_EAST), EF_EXIT);
  if (ISSET(vquint32, bit5))
    orExitFlags(room->exit(ED_WEST), EF_EXIT);
  if (ISSET(vquint32, bit6))
    orExitFlags(room->exit(ED_UP), EF_EXIT);
  if (ISSET(vquint32, bit7))
    orExitFlags(room->exit(ED_DOWN), EF_EXIT);
  if (ISSET(vquint32, bit8))
  {
    orExitFlags(room->exit(ED_NORTH), EF_DOOR);
    orExitFlags(room->exit(ED_NORTH), EF_NO_MATCH);
  }
  if (ISSET(vquint32, bit9))
  {
    orExitFlags(room->exit(ED_SOUTH), EF_DOOR);
    orExitFlags(room->exit(ED_SOUTH), EF_NO_MATCH);
  }
  if (ISSET(vquint32, bit10))
  {
    orExitFlags(room->exit(ED_EAST), EF_DOOR);
    orExitFlags(room->exit(ED_EAST), EF_NO_MATCH);
  }
  if (ISSET(vquint32, bit11))
  {
    orExitFlags(room->exit(ED_WEST), EF_DOOR);
    orExitFlags(room->exit(ED_WEST), EF_NO_MATCH);
  }
  if (ISSET(vquint32, bit12))
  {
    orExitFlags(room->exit(ED_UP), EF_DOOR);
    orExitFlags(room->exit(ED_UP), EF_NO_MATCH);
  }
  if (ISSET(vquint32, bit13))
  {
    orExitFlags(room->exit(ED_DOWN), EF_DOOR);
    orExitFlags(room->exit(ED_DOWN), EF_NO_MATCH);
  }
  if (ISSET(vquint32, bit14))
    orExitFlags(room->exit(ED_NORTH), EF_ROAD);
  if (ISSET(vquint32, bit15))
    orExitFlags(room->exit(ED_SOUTH), EF_ROAD);
  if (ISSET(vquint32, bit16))
    orExitFlags(room->exit(ED_EAST), EF_ROAD);
  if (ISSET(vquint32, bit17))
    orExitFlags(room->exit(ED_WEST), EF_ROAD);
  if (ISSET(vquint32, bit18))
    orExitFlags(room->exit(ED_UP), EF_ROAD);
  if (ISSET(vquint32, bit19))
    orExitFlags(room->exit(ED_DOWN), EF_ROAD);

  stream >> vqint8; //roomUpdated

  stream >> vqint8; //roomCheckExits
  Coordinate pos;
  stream >> (quint32 &)pos.x;
  stream >> (quint32 &)pos.y;

  room->setPosition(pos + basePosition);

  Connection *c;

  stream >> vquint32;
  if (vquint32!=0)
  {
    c = connectionList[vquint32-1];
    if (c->getRoom(0)) c->setRoom(room,1);
    else c->setRoom(room,0);
    c->setDirection(CD_UP, room);
  }
  stream >> vquint32;
  if (vquint32!=0)
  {
    c = connectionList[vquint32-1];
    if (c->getRoom(0)) c->setRoom(room,1);
    else c->setRoom(room,0);
    c->setDirection(CD_DOWN, room);
  }
  stream >> vquint32;
  if (vquint32!=0)
  {
    c = connectionList[vquint32-1];
    if (c->getRoom(0)) c->setRoom(room,1);
    else c->setRoom(room,0);
    c->setDirection(CD_EAST, room);
  }
  stream >> vquint32;
  if (vquint32!=0)
  {
    c = connectionList[vquint32-1];
    if (c->getRoom(0)) c->setRoom(room,1);
    else c->setRoom(room,0);
    c->setDirection(CD_WEST, room);
  }
  stream >> vquint32;
  if (vquint32!=0)
  {
    c = connectionList[vquint32-1];
    if (c->getRoom(0)) c->setRoom(room,1);
    else c->setRoom(room,0);
    c->setDirection(CD_NORTH, room);
  }
  stream >> vquint32;
  if (vquint32!=0)
  {
    c = connectionList[vquint32-1];
    if (c->getRoom(0)) c->setRoom(room,1);
    else c->setRoom(room,0);
    c->setDirection(CD_SOUTH, room);
  }

  // store imported values
  room->replace(R_TERRAINTYPE, terrainType);
  room->replace(R_LIGHTTYPE, lightType);
  room->replace(R_ALIGNTYPE, alignType);
  room->replace(R_PORTABLETYPE, portableType);
  room->replace(R_RIDABLETYPE, ridableType);
  room->replace(R_MOBFLAGS, mobFlags);
  room->replace(R_LOADFLAGS, loadFlags);

  return room;
}


Room * MapStorage::loadRoom(QDataStream & stream, qint32 version)
{
  QString vqba;
  quint32 vquint32;
  quint8 vquint8;
  quint16 vquint16;
  Room * room = factory.createRoom();
  room->setPermanent();

  stream >> vqba;
  room->replace(R_NAME, vqba);
  stream >> vqba;
  room->replace(R_DESC, vqba);
  stream >> vqba;
  room->replace(R_DYNAMICDESC, vqba);
  stream >> vquint32; room->setId(vquint32+baseId);
  stream >> vqba;
  room->replace(R_NOTE, vqba);
  stream >> vquint8;
  room->replace(R_TERRAINTYPE, vquint8);
  stream >> vquint8; room->replace(R_LIGHTTYPE, vquint8);
  stream >> vquint8; room->replace(R_ALIGNTYPE, vquint8);
  stream >> vquint8;
  room->replace(R_PORTABLETYPE, vquint8);  
  if (version >= 030)
	  stream >> vquint8;
  else
	  vquint8 = 0;
  room->replace(R_RIDABLETYPE, vquint8);
  stream >> vquint16; room->replace(R_MOBFLAGS, vquint16);
  stream >> vquint16; room->replace(R_LOADFLAGS, vquint16);

  stream >> vquint8; //roomUpdated
  if (vquint8)
  {
    room->setUpToDate();
  }
  Coordinate c;
  stream >> (qint32 &)c.x;
  stream >> (qint32 &)c.y;
  stream >> (qint32 &)c.z;
  
  room->setPosition(c + basePosition);
  loadExits(room, stream, version);
  return room;
}

void MapStorage::loadExits(Room * room, QDataStream & stream, qint32 /*version*/)
{
  ExitsList & eList = room->getExitsList();
  for (int i = 0; i < 7; ++i)
  {
    Exit & e = eList[i];

    ExitFlags flags;
    stream >> flags;
    if (ISSET(flags, EF_DOOR)) SET(flags, EF_EXIT);
    e[E_FLAGS] = flags;

    DoorFlags dFlags;
    stream >> dFlags;
    e[E_DOORFLAGS] = dFlags;

    DoorName dName;
    stream >> dName;
    e[E_DOORNAME] = dName;

    quint32 connection;
    for (stream >> connection; connection != UINT_MAX; stream >> connection)
    {
      e.addIn(connection+baseId);
    }
    for (stream >> connection; connection != UINT_MAX; stream >> connection)
    {
      e.addOut(connection+baseId);
    }
  }
}

bool MapStorage::loadData()
{
  //clear previous map
  m_mapData.clear();
  return mergeData();
}


bool MapStorage::mergeData()
{
  {
    MapFrontendBlocker blocker(m_mapData);
    
    baseId = m_mapData.getMaxId() + 1;
    basePosition = m_mapData.getLrb();
    if (basePosition.x + basePosition.y + basePosition.z != 0)
    {
	    //basePosition.y++;
	    //basePosition.x = 0;
	    //basePosition.z = 0;
	    basePosition.y = 0;
	    basePosition.x = 0;
	    basePosition.z = -1;
    }
    
    emit log ("MapStorage", "Loading data ...");
    m_progressCounter->reset();

    QDataStream stream (m_file);

    Room *room = NULL;
    InfoMark *mark = NULL;

    quint32 magic;
    qint32 version;
    quint32 index = 0;
    quint32 roomsCount = 0;
    quint32 connectionsCount = 0;
    quint32 marksCount = 0;

    m_mapData.setDataChanged();

    // Read the version and magic
    stream >> magic;
    if ( magic != 0xFFB2AF01 ) return false;
    stream >> version;
    if ( version != 031 && version != 030 &&
	 version != 020 && version != 021 && version != 007 ) return false;

    // QtIOCompressor
    if (version >= 031) {
      m_compressor->open(QIODevice::ReadOnly);
      stream.setDevice(m_compressor);
    }

    stream >> roomsCount;
    if (version < 020) stream >> connectionsCount;
    stream >> marksCount;

    m_progressCounter->increaseTotalStepsBy( roomsCount + connectionsCount + marksCount );

    Coordinate pos;

    //read selected room x,y
    if (version < 020) // OLD VERSIONS SUPPORT CODE
    {
      stream >> (quint32 &)pos.x;
      stream >> (quint32 &)pos.y;
    }
    else
    {
      stream >> (qint32 &)pos.x;
      stream >> (qint32 &)pos.y;
      stream >> (qint32 &)pos.z;
    }

    pos += basePosition;
    
    m_mapData.setPosition(pos);

    emit log ("MapStorage", QString("Number of rooms: %1").arg(roomsCount) );

    ConnectionList connectionList;
    // create all pointers to connections
    for(index = 0; index<connectionsCount; index++)
    {
      Connection * connection = new Connection;
      connectionList.append(connection);
    }

    RoomList roomList(roomsCount);
    for (uint i = 0; i < roomsCount; ++i)
    {
      if (version < 020) // OLD VERSIONS SUPPORT CODE
      {
        room = loadOldRoom(stream, connectionList);
        room->setId(i + baseId);
        roomList[i] = room;
      }
      else
      {
        room = loadRoom(stream, version);
      }

      m_progressCounter->step();
      m_mapData.insertPredefinedRoom(room);
    }

    if (version < 020)
    {
      emit log ("MapStorage", QString("Number of connections: %1").arg(connectionsCount) );
      ConnectionListIterator c(connectionList);
      while (c.hasNext())
      {
        Connection * connection = c.next();
        loadOldConnection(connection, stream, roomList);
        translateOldConnection(connection);
        delete connection;

        m_progressCounter->step();
      }
      connectionList.clear();
    }

    emit log ("MapStorage", QString("Number of info items: %1").arg(marksCount) );


    MarkerList& markerList = m_mapData.getMarkersList();
    // create all pointers to items
    for(index = 0; index<marksCount; index++)
    {
      mark = new InfoMark();
      loadMark(mark, stream, version);      
      markerList.append(mark);
      
      m_progressCounter->step();
    }

    emit log ("MapStorage", "Finished loading.");

    if (m_mapData.getRoomsCount() < 1 ) return false;

    m_mapData.setFileName(m_fileName);
    m_mapData.unsetDataChanged();

    if (version >= 031)
      m_compressor->close();

  }

  m_mapData.checkSize();
  emit onDataLoaded();
  return true;
}



void MapStorage::loadMark(InfoMark * mark, QDataStream & stream, qint32 version)
{
  QString vqstr;
  quint16 vquint16;
  quint32 vquint32;
  qint32  vqint32;

  qint32 postfix = basePosition.x + basePosition.y + basePosition.z;

  if (version < 020) // OLD VERSIONS SUPPORT CODE
  {
    stream >> vqstr; 
    if (postfix != 0 && postfix != 1)
    {
    	vqstr += QString("_m%1").arg(postfix);
    }
    mark->setName(vqstr);
    stream >> vqstr; mark->setText(vqstr);
    stream >> vquint16; mark->setType((InfoMarkType)vquint16);

    Coordinate pos;
    stream >> vquint32; pos.x = (qint32) vquint32;
    pos.x = pos.x*100/48-40;
    stream >> vquint32; pos.y = (qint32) vquint32;
    pos.y = pos.y*100/48-55;
    //pos += basePosition;
    pos.x += basePosition.x*100;
    pos.y += basePosition.y*100;
    pos.z += basePosition.z;
    mark->setPosition1(pos);

    stream >> vquint32; pos.x = (qint32) vquint32;
    pos.x = pos.x*100/48-40;
    stream >> vquint32; pos.y = (qint32) vquint32;
    pos.y = pos.y*100/48-55;
    //pos += basePosition;
    pos.x += basePosition.x*100;
    pos.y += basePosition.y*100;
    pos.z += basePosition.z;
    mark->setPosition2(pos);
  }
  else
  {
    QString vqba;
    QDateTime vdatetime;
    quint8 vquint8;

    stream >> vqba; 
    if (postfix != 0 && postfix != 1)
    {
    	vqba += QString("_m%1").arg(postfix).toAscii();
    }   
    mark->setName(vqba);
    stream >> vqba; mark->setText(vqba);
    stream >> vdatetime; mark->setTimeStamp(vdatetime);
    stream >> vquint8; mark->setType((InfoMarkType)vquint8);

    Coordinate pos;
    stream >> vqint32; pos.x = vqint32/*-40*/;
    stream >> vqint32; pos.y = vqint32/*-55*/;
    stream >> vqint32; pos.z = vqint32;
    pos.x += basePosition.x*100;
    pos.y += basePosition.y*100;
    pos.z += basePosition.z;
    //pos += basePosition;
    mark->setPosition1(pos);

    stream >> vqint32; pos.x = vqint32/*-40*/;
    stream >> vqint32; pos.y = vqint32/*-55*/;
    stream >> vqint32; pos.z = vqint32;
    pos.x += basePosition.x*100;
    pos.y += basePosition.y*100;
    pos.z += basePosition.z;
    //pos += basePosition;
    mark->setPosition2(pos);
  }
}


void MapStorage::translateOldConnection(Connection * c)
{
  Room * left = c->getRoom(LEFT);
  Room * right = c->getRoom(RIGHT);
  ConnectionDirection leftDir = c->getDirection(LEFT);
  ConnectionDirection rightDir = c->getDirection(RIGHT);
  ConnectionFlags cFlags = c->getFlags();

  if (leftDir != CD_NONE)
  {
    Exit & e = left->exit(leftDir);
    e.addOut(right->getId());
    ExitFlags eFlags = getFlags(e);
    if (cFlags & CF_DOOR)
    {
      eFlags |= EF_NO_MATCH;
      eFlags |= EF_DOOR;
      e[E_DOORNAME] = c->getDoor(LEFT)->getName();
      e[E_DOORFLAGS] = c->getDoor(LEFT)->getFlags();
    }
    if (cFlags & CF_RANDOM) eFlags |= EF_RANDOM;
    if (cFlags & CF_CLIMB) eFlags |= EF_CLIMB;
    if (cFlags & CF_SPECIAL) eFlags |= EF_SPECIAL;
    eFlags |= EF_EXIT;
    e[E_FLAGS] = eFlags;

    Exit & eR = right->exit(opposite(leftDir));
    eR.addIn(left->getId());
  }
  if (rightDir != CD_NONE)
  {
    Exit & eL = left->exit(opposite(rightDir));
    eL.addIn(right->getId());

    Exit & e = right->exit(rightDir);
    e.addOut(left->getId());
    ExitFlags eFlags = getFlags(e);
    if (cFlags & CF_DOOR)
    {
      eFlags |= EF_DOOR;
      eFlags |= EF_NO_MATCH;
      e[E_DOORNAME] = c->getDoor(RIGHT)->getName();
      e[E_DOORFLAGS] = c->getDoor(RIGHT)->getFlags();
    }
    if (cFlags & CF_RANDOM) eFlags |= EF_RANDOM;
    if (cFlags & CF_CLIMB) eFlags |= EF_CLIMB;
    if (cFlags & CF_SPECIAL) eFlags |= EF_SPECIAL;
    eFlags |= EF_EXIT;
    e[E_FLAGS] = eFlags;
  }
}


void MapStorage::loadOldConnection(Connection * connection, QDataStream & stream, RoomList & roomList)
{
  quint16 vquint16;
  QString vqstr;
  quint32 vquint32;

  Room *r1 = 0, *r2 = 0;

  ConnectionFlags cf = 0;

  connection->setNote("");

  stream >> vquint16;
  ConnectionType ct = (ConnectionType)(vquint16 & (bit1 + bit2));
  cf = (vquint16 >> 2);
  /*
  switch (vquint16)
  {
  case 0: ct = CT_NORMAL ; break;  // connection normal
  case 1: ct = CT_LOOP   ; break;  // loop
  case 2: ct = CT_ONEWAY ; break;  // oneway
  case 4:  SET( cf, CF_DOOR)   ; break; // door
  case 8:  SET( cf, CF_CLIMB)  ; break; // climb
  case 16: SET( cf, CF_SPECIAL); break; // special
  case 32: SET( cf, CF_RANDOM) ; break; // random
  }
  */
  // DOOR TYPE: DNORMAL, DHIDDEN, DNORMALNEEDKEY, DHIDDENNEEDKEY
  DoorFlags doorFlags1 = 0, doorFlags2 = 0;
  DoorName doorName1, doorName2;
  stream >> vquint16;
  switch (vquint16)
  {
  case 0: break;                      //Normal
  case 1: SET(doorFlags1, DF_HIDDEN); //Hidden
    break;
  case 2: SET(doorFlags1, DF_NEEDKEY); //Normal+Key
    break;
  case 3: SET(doorFlags1, DF_HIDDEN); //Hidden+Key
    SET(doorFlags1, DF_NEEDKEY);
    break;
  }
  stream >> vquint16;
  switch (vquint16)
  {
  case 0: break;                      //Normal
  case 1: SET(doorFlags2, DF_HIDDEN); //Hidden
    break;
  case 2: SET(doorFlags2, DF_NEEDKEY); //Normal+Key
    break;
  case 3: SET(doorFlags2, DF_HIDDEN); //Hidden+Key
    SET(doorFlags2, DF_NEEDKEY);
    break;
  }

  stream >> vqstr; doorName1=vqstr;
  stream >> vqstr; doorName2=vqstr;

  stream >> vquint32;
  if (vquint32!=0 && ( (vquint32-1) < (uint)roomList.size()) )
  {
    r1 = roomList[vquint32-1];
    if ( ISSET(cf, CF_DOOR) )
      connection->setDoor(new Door(doorName1, doorFlags1), LEFT);
  }

  stream >> vquint32;
  if (vquint32!=0 && ( (vquint32-1) < (uint)roomList.size()) )
  {
    r2 = roomList[vquint32-1];
    if ( ISSET(cf, CF_DOOR) )
      connection->setDoor(new Door(doorName2, doorFlags2), RIGHT);
  }

  assert(r1 != 0); // if these are not valid,
  assert(r2 != 0); // the indices were out of bounds

  Room * room = 0;
  if (!connection->getRoom(0))
  {
    room = connection->getRoom(1);
    if (room != r1)
    {
      connection->setRoom(r1, 0);
    }
    else
    {
      connection->setRoom(r2, 0);
    }
  }
  if(!connection->getRoom(1))
  {
    room = connection->getRoom(0);
    if (room != r1)
    {
      connection->setRoom(r1, 1);
    }
    else
    {
      connection->setRoom(r2, 1);
    }
  }

  assert(connection->getRoom(0) != 0);
  assert(connection->getRoom(1) != 0);


  if (ISSET(cf, CF_DOOR))
  {
    assert(connection->getDoor(LEFT) != NULL);
    assert(connection->getDoor(RIGHT) != NULL);
  }

  if (connection->getDirection(RIGHT) == CD_UNKNOWN)
  {
    ct = CT_ONEWAY;
    connection->setDirection(CD_NONE, RIGHT);
  }
  else if(connection->getDirection(LEFT) == CD_UNKNOWN)
  {
    ct = CT_ONEWAY;
    connection->setDirection(connection->getDirection(RIGHT), LEFT);
    connection->setDirection(CD_NONE, LEFT);
    Room * temp = connection->getRoom(LEFT);
    connection->setRoom(connection->getRoom(RIGHT), LEFT);
    connection->setRoom(temp, RIGHT);
  }

  if (connection->getRoom(0) == connection->getRoom(1))
    ct = CT_LOOP;

  connection->setType(ct);
  connection->setFlags(cf);
}

void MapStorage::saveRoom(const Room * room, QDataStream & stream)
{
  stream << getName(room);
  stream << getDescription(room);
  stream << getDynamicDescription(room);
  stream << (quint32)room->getId();
  stream << getNote(room);
  stream << (quint8)getTerrainType(room);
  stream << (quint8)getLightType(room);
  stream << (quint8)getAlignType(room);
  stream << (quint8)getPortableType(room);
  stream << (quint8)getRidableType(room);
  stream << (quint16)getMobFlags(room);
  stream << (quint16)getLoadFlags(room);

  stream << (quint8)room->isUpToDate();

  const Coordinate & pos = room->getPosition();
  stream << (qint32)pos.x;
  stream << (qint32)pos.y;
  stream << (qint32)pos.z;
  saveExits(room, stream);
}

void MapStorage::saveExits(const Room * room, QDataStream & stream)
{
  const ExitsList& exitList = room->getExitsList();
  ExitsListIterator el(exitList);
  while (el.hasNext())
  {
    const Exit & e = el.next();
    stream << getFlags(e);
    stream << getDoorFlags(e);
    stream << getDoorName(e);
    for (set<uint>::const_iterator i = e.inBegin(); i != e.inEnd(); ++i)
    {
      stream << (quint32)*i;
    }
    stream << (quint32)UINT_MAX;
    for (set<uint>::const_iterator i = e.outBegin(); i != e.outEnd(); ++i)
    {
      stream << (quint32)*i;
    }
    stream << (quint32)UINT_MAX;
  }
}

bool MapStorage::saveData( bool baseMapOnly )
{
  emit log ("MapStorage", "Writing data to file ...");

  QDataStream stream (m_file);

  // Collect the room and marker lists. The room list can't be acquired
  // directly apparently and we have to go through a RoomSaver which receives
  // them from a sort of callback function.
  QList<const Room *> roomList;
  MarkerList& markerList = m_mapData.getMarkersList();
  RoomSaver saver(&m_mapData, roomList);
  for (uint i = 0; i < m_mapData.getRoomsCount(); ++i)
  {
    m_mapData.lookingForRooms(&saver, i);
  }

  uint roomsCount = saver.getRoomsCount();
  uint marksCount = markerList.size();

  m_progressCounter->reset();
  m_progressCounter->increaseTotalStepsBy( roomsCount + marksCount );

  BaseMapSaveFilter filter;
  if ( baseMapOnly )
  {
      filter.setMapData( &m_mapData );
      m_progressCounter->increaseTotalStepsBy( filter.prepareCount() );
      filter.prepare( m_progressCounter );
      roomsCount = filter.acceptedRoomsCount();
  }

  // Write a header with a "magic number" and a version
  stream << (quint32)0xFFB2AF01;
  stream << (qint32)031;

  // QtIOCompressor
  m_compressor->open(QIODevice::WriteOnly);
  stream.setDevice(m_compressor);

  //write counters
  stream << (quint32)roomsCount;
  stream << (quint32)marksCount;

  //write selected room x,y
  Coordinate & self = m_mapData.getPosition();
  stream << (qint32)self.x;
  stream << (qint32)self.y;
  stream << (qint32)self.z;

  // save rooms
  QListIterator<const Room *>  roomit(roomList);
  while (roomit.hasNext())
  {
    const Room *room = roomit.next();
    if ( baseMapOnly )
    {
      BaseMapSaveFilter::Action action = filter.filter( room );
      if ( !room->isTemporary() && action != BaseMapSaveFilter::REJECT )
      {
        if ( action == BaseMapSaveFilter::ALTER )
        {
          Room copy = filter.alteredRoom( room );
          saveRoom( &copy, stream );
        }
        else // action == PASS
        {
          saveRoom(room, stream);
        }
      }
    }
    else
    {
      saveRoom(room, stream);
    }

    m_progressCounter->step();
  }

  // save items
  MarkerListIterator markerit(markerList);
  while (markerit.hasNext())
  {
    InfoMark *mark = markerit.next();
    saveMark(mark, stream);

    m_progressCounter->step();
  }
  
  m_compressor->close();
  
  emit log ("MapStorage", "Writting data finished.");

  m_mapData.unsetDataChanged();
  emit onDataSaved();

  return TRUE;
}


void MapStorage::saveMark(InfoMark * mark, QDataStream & stream)
{
  stream << (QString)mark->getName();
  stream << (QString)mark->getText();
  stream << (QDateTime)mark->getTimeStamp();
  stream << (quint8)mark->getType();
  const Coordinate & c1 = mark->getPosition1();
  const Coordinate & c2 = mark->getPosition2();
  stream << (qint32)c1.x;
  stream << (qint32)c1.y;
  stream << (qint32)c1.z;
  stream << (qint32)c2.x;
  stream << (qint32)c2.y;
  stream << (qint32)c2.z;
}

