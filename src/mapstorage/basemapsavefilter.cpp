/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
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

#include <set>
#include <list>
#include <cassert>
#include <algorithm>
#include <functional>

#include "mapdata/mapdata.h"
#include "mapdata/mmapper2exit.h"
#include "mapstorage/basemapsavefilter.h"
#include "mapstorage/progresscounter.h"

using namespace std;

namespace
{
  class IsSecret
  {
    const std::set<uint> &m_baseRooms;

  public:
    IsSecret( const set<uint> &baseRooms ) : m_baseRooms( baseRooms ) {}

    typedef uint argument_type;

    bool operator() ( argument_type roomId )
    {
      return m_baseRooms.find( roomId ) == m_baseRooms.end();
    }
  };

  template< typename Container >
  class InsertInto
  {
    Container &m_container;

  public:
    InsertInto( Container &container ) : m_container( container ) {}

    void operator() ( typename Container::value_type element )
    {
      m_container.insert( element );
    }
  };

  bool isHiddenExit( const Exit &exit )
  {
    return ISSET( getFlags( exit ), EF_DOOR ) && ISSET( getDoorFlags( exit ), DF_HIDDEN );
  }

  struct RoomLink
  {
    uint from, to;
    RoomLink( uint f, uint t ) : from( f ), to( t ) {}

  private:
    RoomLink();
  };

  bool operator<( const RoomLink &a, const RoomLink &b )
  {
    if ( a.from != b.from )
      return a.from < b.from;
    else
      return a.to < b.to;
  }
}

struct BaseMapSaveFilter::Impl
{
  //! Owned by caller
  MapData *mapData;
  
  //! Rooms reachable without going through hidden doors
  std::set<uint> baseRooms;
  
  /*! \brief Rooms scheduled for exploration during prepare
   *
   * Order matters (can't be replaced by a set) to prevent infinite looping.
   */
  std::list<uint> roomsTodo;

  /*! \brief Secret links noticed during the exploration (prepare).
   *
   * This data is used to remove secret links between public rooms (example:
   * hedge on OER near Bree). As such it doesn't include secret links between
   * secret rooms, but this doesn't matter.
   */
  std::set<RoomLink> secretLinks;
};

BaseMapSaveFilter::BaseMapSaveFilter()
 : m_impl( new Impl )
{
}

BaseMapSaveFilter::~BaseMapSaveFilter()
{
}

void BaseMapSaveFilter::prepare( ProgressCounter *counter )
{
  assert( m_impl->mapData );

  set<uint> considered;

  // Seed room
  m_impl->mapData->searchNames( this, "The Fountain Square", Qt::CaseSensitive );

  // Walk the whole map through non-hidden exits without recursing
  while ( !m_impl->roomsTodo.empty() )
  {
    uint todo = m_impl->roomsTodo.front();
    m_impl->roomsTodo.pop_front();

    if ( considered.find( todo ) == considered.end() ) // Don't process twice the same room (ending condition)
    {
      m_impl->mapData->lookingForRooms( this, todo );
      m_impl->mapData->releaseRoom( this, todo );
      considered.insert( todo );
      counter->step();
    }
  }

  for ( uint steps = considered.size(); steps < prepareCount(); ++steps )
    counter->step(); // Make up for the secret rooms we skipped
}

void BaseMapSaveFilter::receiveRoom( RoomAdmin *, const Room * room )
{
  const ExitsList &exits( room->getExitsList() );
  for ( ExitsList::const_iterator exit = exits.begin(); exit != exits.end(); ++exit )
  {
    if ( isHiddenExit( *exit ) )
    {
      for ( set<uint>::const_iterator to = exit->outBegin(); to != exit->outEnd(); ++to )
        m_impl->secretLinks.insert( RoomLink( room->getId(), *to ) );
    }
    else
    {
      for_each( exit->outBegin(), exit->outEnd(), InsertInto< set<uint> >( m_impl->baseRooms ) );
      copy( exit->outBegin(), exit->outEnd(), back_inserter( m_impl->roomsTodo ) );
    }
  }
}

void BaseMapSaveFilter::setMapData( MapData *mapData )
{
  m_impl->mapData = mapData;
}

uint BaseMapSaveFilter::prepareCount()
{
  assert( m_impl->mapData );
  return m_impl->mapData->getRoomsCount();
}

uint BaseMapSaveFilter::acceptedRoomsCount()
{
  return m_impl->baseRooms.size();
}

BaseMapSaveFilter::Action BaseMapSaveFilter::filter( const Room *room )
{
  assert( !m_impl->baseRooms.empty() );

  if ( m_impl->baseRooms.find( room->getId() ) != m_impl->baseRooms.end() )
  {
    const ExitsList &exits = room->getExitsList();
    for ( ExitsList::const_iterator exit = exits.begin(); exit != exits.end(); ++exit )
    {
      if ( isHiddenExit( *exit ) )
        return ALTER;
      if ( find_if( exit->outBegin(), exit->outEnd(), IsSecret( m_impl->baseRooms ) ) != exit->outEnd() )
        return ALTER;
      if ( find_if( exit->inBegin(), exit->inEnd(), IsSecret( m_impl->baseRooms ) ) != exit->inEnd() )
        return ALTER;
    }
    
    return PASS;
  }
  else
  {
    return REJECT;
  }
}

Room BaseMapSaveFilter::alteredRoom( const Room *room )
{
  assert( !m_impl->baseRooms.empty() );

  Room copy = *room;

  ExitsList &exits = copy.getExitsList();
  for ( ExitsList::iterator exit = exits.begin(); exit != exits.end(); ++exit )
  {
    vector<uint> outLinks( exit->outBegin(), exit->outEnd() );
    vector<uint> inLinks( exit->inBegin(), exit->inEnd() );

    // Destroy links to secret rooms
    for ( uint i = 0; i < outLinks.size(); ++i )
    {
      const bool destRoomIsSecret = IsSecret( m_impl->baseRooms )( outLinks[i] );
      const bool outLinkIsSecret = (
        m_impl->secretLinks.find( RoomLink( copy.getId(), outLinks[i] ) ) != m_impl->secretLinks.end() );
      const bool linkBackIsSecret = (
        m_impl->secretLinks.find( RoomLink( outLinks[i], copy.getId() ) ) != m_impl->secretLinks.end() );

      if ( destRoomIsSecret || ( outLinkIsSecret && linkBackIsSecret ) )
      {
        exit->removeOut( outLinks[i] );

        (*exit)[E_DOORNAME] = "";
        (*exit)[E_FLAGS] = 0;
        (*exit)[E_DOORFLAGS] = 0;

      }
    }

    // Destroy links from secret rooms to here
    for ( uint i = 0; i < inLinks.size(); ++i )
      if ( IsSecret( m_impl->baseRooms )( inLinks[i] ) )
        exit->removeIn( inLinks[i] );

    // Remove names on hidden exits to areas reachable through visible doors
    if ( isHiddenExit( *exit ) )
    {
      (*exit)[E_DOORNAME] = "";
    }
  }

  return copy;
}

