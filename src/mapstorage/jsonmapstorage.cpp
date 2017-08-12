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

#include "jsonmapstorage.h"
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
#include <QDir>
#include <QTextStream>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QMultiMap>
#include <QCryptographicHash>

#include <cassert>
#include <iostream>

using namespace std;

namespace
{

typedef QList<const Room *> RoomList2;

// These settings have to be shared with the JS code:
// Group all rooms with the same 2 first hash bytes into the same file
const int c_roomIndexFileNameSize = 2;
// Split the world into 20x20 zones
const int c_zoneWidth = 20;

/* Performs MD5 hashing on the string with its non-ASCII chars removed. This
 * was the simplest scheme that could be implemented in the JavaScript
 * counterpart.
 */
class WebHasher
{
  QCryptographicHash m_hash;

public:
  WebHasher()
   : m_hash( QCryptographicHash::Md5 )
  {
  }

  void add( QString str )
  {
    QByteArray bytes;
    for ( int i = 0; i < str.length(); ++i )
    {
      if ( str[i] < 128 )
        bytes.append( str[i].toLatin1() );
    }

    m_hash.addData(bytes);
  }

  QByteArray result() const
  {
    return m_hash.result();
  }

  void reset()
  {
    m_hash.reset();
  }
};

// Lets the webclient locate and load the useful zones only, not the whole
// world at once.
class RoomHashIndex
{
public:
  typedef QMultiMap<QByteArray, Coordinate> Index;

private:
  Index m_index;
  WebHasher m_hasher;

public:
  void addRoom( const Room &room )
  {
    m_hasher.add( getName( &room ) );
    m_hasher.add( getDescription( &room ) );
    m_index.insert( m_hasher.result().toHex(), room.getPosition() );
    m_hasher.reset();
  }

  const Index &index() const
  {
    return m_index;
  }
};

// Splits the world in zones easier to download and load
class ZoneIndex
{
public:
  typedef QMap<QString, RoomList2> Index;

private:
  Index m_index;

public:
  void addRoom( const Room *room )
  {
    Coordinate coords = room->getPosition();
    QString zone( QString( "%1,%2" )
      .arg( coords.x - coords.x % c_zoneWidth )
      .arg( coords.y - coords.y % c_zoneWidth ) );

    Index::iterator iter = m_index.find( zone );
    if ( iter == m_index.end() )
    {
      RoomList2 list;
      list.push_back( room );
      m_index.insert( zone, list );
    }
    else
    {
      iter.value().push_back( room );
    }
  }

  const Index &index() const
  {
    return m_index;
  }
};

template<typename JsonT>
void writeJson( QString filePath, JsonT json, QString what )
{
  QFile file( filePath );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    QString msg( QString( "error opening %1 to %2: %3" )
        .arg( what ).arg( filePath ).arg( file.errorString() ) );
    throw std::runtime_error( msg.toStdString() );
  }

  QJsonDocument doc( json );
  QTextStream stream( &file );
  stream << doc.toJson();
  stream.flush();
  if ( !file.flush() || stream.status() != QTextStream::Ok )
  {
    QString msg( QString( "error writing %1 to %2: %3" )
        .arg( what ).arg( filePath ).arg( file.errorString() ) );
    throw std::runtime_error( msg.toStdString() );
  }
}

class RoomIndexStore
{
  const QDir m_dir;
  QJsonObject m_hashes;
  QByteArray m_prefix;

  RoomIndexStore(); // Disabled

public:
  RoomIndexStore( QDir dir )
   : m_dir( dir )
  {
  }

  void add( QByteArray hash, Coordinate coords )
  {
    QByteArray prefix = hash.left( c_roomIndexFileNameSize );
    if ( m_prefix != prefix )
    {
      close();
      m_prefix = prefix;
    }

    QJsonArray jCoords;
    jCoords.push_back( coords.x );
    jCoords.push_back( coords.y );

    QJsonObject::iterator collision = m_hashes.find( hash );
    if ( collision == m_hashes.end() )
    {
      QJsonArray array;
      array.push_back( jCoords );
      m_hashes.insert( hash, array );
    }
    else
    {
      collision.value().toArray().push_back( jCoords );
    }
  }

  void close()
  {
    if ( m_hashes.isEmpty() )
      return;

    assert( m_prefix.size() != 0 );
    QString filePath = m_dir.filePath( QString::fromLocal8Bit( m_prefix ) + ".json" );

    writeJson( filePath, m_hashes, "room index" );

    m_hashes = QJsonObject();
    m_prefix = QByteArray();
  }
};


// Maps MM2 room IDs -> hole-free JSON room IDs
class JsonRoomIdsCache
{
  typedef QMap<uint, uint> CacheT;
  CacheT m_cache;
  uint m_nextJsonId;

public:
  JsonRoomIdsCache();
  void addRoom( uint mm2RoomId )
  {
    m_cache[mm2RoomId] = m_nextJsonId++;
  }
  uint operator[](uint roomId) const;
  uint size() const;
};

JsonRoomIdsCache::JsonRoomIdsCache()
 : m_nextJsonId(0)
{
}

uint JsonRoomIdsCache::operator[](uint roomId) const
{
  CacheT::const_iterator it = m_cache.find(roomId);
  assert(it != m_cache.end());
  return *it;
}

uint JsonRoomIdsCache::size() const
{
    return m_nextJsonId;
}


// Expects that a RoomSaver locks the Rooms for the lifetime of this object!
class JsonWorld
{
  JsonRoomIdsCache m_jRoomIds;
  RoomHashIndex m_roomHashIndex;
  ZoneIndex m_zoneIndex;

  void addRoom( QJsonArray &jRooms, const Room *room ) const;
  void addExits( const Room * room, QJsonObject &jr ) const;

public:
  JsonWorld();
  ~JsonWorld();
  void addRooms( const RoomList2 &roomList, BaseMapSaveFilter &filter,
    QPointer<ProgressCounter> progressCounter, bool baseMapOnly );
  void writeMetadata( const QFileInfo &path, const MapData &mapData ) const;
  void writeRoomIndex( const QDir &dir ) const;
  void writeZones( const QDir &dir, BaseMapSaveFilter &filter,
    QPointer<ProgressCounter> progressCounter, bool baseMapOnly ) const;
};

JsonWorld::JsonWorld()
{
}

JsonWorld::~JsonWorld()
{
}

void JsonWorld::addRooms( const RoomList2 &roomList, BaseMapSaveFilter &filter,
  QPointer<ProgressCounter> progressCounter, bool baseMapOnly )
{
  for ( RoomList2::const_iterator roomIter = roomList.begin();
    roomIter != roomList.end(); ++roomIter )
  {
    progressCounter->step();

    const Room *room = *roomIter;
    if ( baseMapOnly )
    {
      BaseMapSaveFilter::Action action = filter.filter( room );
      if ( room->isTemporary() || action == BaseMapSaveFilter::REJECT )
        continue;
    }

    m_jRoomIds.addRoom( room->getId() );
    m_roomHashIndex.addRoom( *room );
    m_zoneIndex.addRoom( room );
  }
}

void JsonWorld::writeMetadata( const QFileInfo &path, const MapData &mapData ) const
{
  QJsonObject meta;
  meta["roomsCount"] = static_cast<qint64>(m_jRoomIds.size());
  meta["minX"] = mapData.getUlf().x;
  meta["minY"] = mapData.getUlf().y;
  meta["minZ"] = mapData.getUlf().z;
  meta["maxX"] = mapData.getLrb().x;
  meta["maxY"] = mapData.getLrb().y;
  meta["maxZ"] = mapData.getLrb().z;
  QJsonArray directions;
  for ( size_t i = 0; i <= ED_NONE; ++i ) // resize array
    directions.push_back( QJsonValue() );
  directions[ED_NORTH]   = "NORTH";
  directions[ED_SOUTH]   = "SOUTH";
  directions[ED_EAST]    = "EAST";
  directions[ED_WEST]    = "WEST";
  directions[ED_UP]      = "UP";
  directions[ED_DOWN]    = "DOWN";
  directions[ED_UNKNOWN] = "UNKNOWN";
  directions[ED_NONE]    = "NONE";
  meta["directions"] = directions;

  writeJson( path.filePath(), meta, "metadata" );
}

void JsonWorld::writeRoomIndex( const QDir &dir ) const
{
  typedef RoomHashIndex::Index::const_iterator IterT;
  const RoomHashIndex::Index &index = m_roomHashIndex.index();

  RoomIndexStore store( dir );
  for ( IterT iter = index.cbegin(); iter != index.cend(); ++iter )
    store.add( iter.key(), iter.value() );
  store.close();
}

void JsonWorld::addRoom( QJsonArray &jRooms, const Room *room ) const
{
  /*
        x: 5, y: 5, z: 0,
        north: null, east: 1, south: null, west: null, up: null, down: null,
        sector: 2 * SECT_CITY *, mobflags: 0, loadflags: 0, light: null, RIDEABLE: null,
        name: "Fortune's Delving",
        desc:
        "A largely ceremonial hall, it was the first mineshaft that led down to what is\n"
  */

  const Coordinate & pos = room->getPosition();
  QJsonObject jr;
  jr["x"] = pos.x;
  jr["y"] = pos.y;
  jr["z"] = pos.z;

  uint jsonId = m_jRoomIds[room->getId()];
  jr["id"]        = QString::number(jsonId);
  jr["name"]      = getName(room);
  jr["desc"]      = getDescription(room);
  jr["sector"]    = (quint8)getTerrainType(room);
  jr["light"]     = (quint8)getLightType(room);
  jr["portable"]  = (quint8)getPortableType(room);
  jr["rideable"]  = (quint8)getRidableType(room);
  jr["mobflags"]  = (quint16)getMobFlags(room);
  jr["loadflags"] = (quint16)getLoadFlags(room);

  addExits( room, jr );

  jRooms.push_back( jr );
}

void JsonWorld::addExits( const Room * room, QJsonObject &jr ) const
{
  const ExitsList& exitList = room->getExitsList();
  ExitsListIterator el(exitList);
  QJsonArray jExits; // Direction-indexed
  while (el.hasNext())
  {
    const Exit & e = el.next();
    QJsonObject je;
    je["flags"]  = getFlags(e);
    je["dflags"] = getDoorFlags(e);
    je["name"]   = getDoorName(e);

    QJsonArray jin;
    for (set<uint>::const_iterator i = e.inBegin(); i != e.inEnd(); ++i)
    {
      jin << QString::number(m_jRoomIds[*i]);
    }
    je["in"] = jin;

    QJsonArray jout;
    for (set<uint>::const_iterator i = e.outBegin(); i != e.outEnd(); ++i)
    {
      jout << QString::number(m_jRoomIds[*i]);
    }
    je["out"] = jout;

    jExits << je;
  }

  jr["exits"] = jExits;
}

void JsonWorld::writeZones( const QDir &dir, BaseMapSaveFilter &filter,
  QPointer<ProgressCounter> progressCounter, bool baseMapOnly ) const
{
  typedef ZoneIndex::Index::const_iterator ZoneIterT;
  const ZoneIndex::Index &index = m_zoneIndex.index();

  for ( ZoneIterT zIter = index.cbegin(); zIter != index.cend(); ++zIter )
  {
    const RoomList2 &rooms = zIter.value();
    QJsonArray jRooms;
    for ( RoomList2::const_iterator rIter = rooms.cbegin(); rIter != rooms.cend(); ++rIter )
    {
      if ( baseMapOnly )
      {
        BaseMapSaveFilter::Action action = filter.filter( *rIter );
        if ( action == BaseMapSaveFilter::ALTER )
        {
          Room copy = filter.alteredRoom( *rIter );
          addRoom( jRooms, &copy );
        }
        else
        {
          assert( action == BaseMapSaveFilter::PASS );
          addRoom( jRooms, *rIter );
        }
      }
      else
      {
        addRoom( jRooms, *rIter );
      }
      progressCounter->step();
    }

    QString filePath = dir.filePath( zIter.key() + ".json" );
    writeJson( filePath, jRooms, "zone" );
  }
}

} // ns


JsonMapStorage::JsonMapStorage(MapData& mapdata, const QString& filename) :
    AbstractMapStorage(mapdata, filename)
{}

JsonMapStorage::~JsonMapStorage()
{
}

void JsonMapStorage::newData()
{
  assert("JsonMapStorage does not implement newData()" != NULL);
}

bool JsonMapStorage::loadData()
{
  return false;
}

bool JsonMapStorage::mergeData()
{
  return false;
}

bool JsonMapStorage::saveData( bool baseMapOnly )
{
  emit log ("JsonMapStorage", "Writing data to files ...");


  // Collect the room and marker lists. The room list can't be acquired
  // directly apparently and we have to go through a RoomSaver which receives
  // them from a sort of callback function.
  // The RoomSaver acts as a lock on the rooms.
  RoomList2 roomList;
  MarkerList& markerList = m_mapData.getMarkersList();
  RoomSaver saver(&m_mapData, roomList);
  for (uint i = 0; i < m_mapData.getRoomsCount(); ++i)
  {
    m_mapData.lookingForRooms(&saver, i);
  }

  uint roomsCount = saver.getRoomsCount();
  uint marksCount = markerList.size();
  m_progressCounter->reset();
  m_progressCounter->increaseTotalStepsBy( roomsCount * 2 + marksCount );

  BaseMapSaveFilter filter;
  if ( baseMapOnly )
  {
      filter.setMapData( &m_mapData );
      m_progressCounter->increaseTotalStepsBy( filter.prepareCount() );
      filter.prepare( m_progressCounter );
  }

  JsonWorld world;
  world.addRooms( roomList, filter, m_progressCounter, baseMapOnly );

  QDir saveDir( m_fileName );
  QDir destDir( QFileInfo( saveDir, "v1" ).filePath() );
  QDir roomIndexDir( QFileInfo( destDir, "roomindex" ).filePath() );
  QDir zoneDir( QFileInfo( destDir, "zone" ).filePath() );
  try {
    if ( !saveDir.mkdir( "v1" ) )
      throw std::runtime_error( "error creating dir v1" );
    if ( !destDir.mkdir( "roomindex" ) )
      throw std::runtime_error( "error creating dir v1/roomindex" );
    if ( !destDir.mkdir( "zone" ) )
      throw std::runtime_error( "error creating dir v1/zone" );

    world.writeMetadata( QFileInfo( destDir, "arda.json" ), m_mapData );
    world.writeRoomIndex( roomIndexDir );
    world.writeZones( zoneDir, filter, m_progressCounter, baseMapOnly );
  }
  catch ( std::exception &e )
  {
    emit log( "JsonMapStorage", e.what() );
    return false;
  }

  emit log ("JsonMapStorage", "Writing data finished.");

  m_mapData.unsetDataChanged();
  emit onDataSaved();

  return true;
}


  /*
void JsonMapStorage::saveMark(InfoMark * mark, QJsonObject &jRoom, const JsonRoomIdsCache &jRoomIds)
{
  stream << (QString)mark->getName();
  stream << (QString)mark->getText();
  stream << (QDateTime)mark->getTimeStamp();
  stream << (quint8)mark->getType();
  stream << (quint8)mark->getClass();
  stream << (qint32)(mark->getRotationAngle()*100);
  const Coordinate & c1 = mark->getPosition1();
  const Coordinate & c2 = mark->getPosition2();
  stream << (qint32)c1.x;
  stream << (qint32)c1.y;
  stream << (qint32)c1.z;
  stream << (qint32)c2.x;
  stream << (qint32)c2.y;
  stream << (qint32)c2.z;
}
  */
