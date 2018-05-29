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

#include <algorithm>
#include <cassert>
#include <functional>
#include <list>
#include <set>

#include "global/range.h"
#include "mapdata/mapdata.h"
#include "mapdata/mmapper2exit.h"
#include "mapstorage/basemapsavefilter.h"
#include "mapstorage/progresscounter.h"

namespace {

bool contains(const std::set<uint> &set, uint x)
{
    return set.find(x) == set.end();
}

bool isHiddenExit( const Exit &exit )
{
    return ISSET( Mmapper2Exit::getFlags( exit ), EF_DOOR )
           && ISSET( Mmapper2Exit::getDoorFlags( exit ), DF_HIDDEN );
}

struct RoomLink {
    uint from, to;
    RoomLink( uint f, uint t ) : from( f ), to( t ) {}

public:
    RoomLink() = delete;
};

bool operator<( const RoomLink &a, const RoomLink &b )
{
    if ( a.from != b.from ) {
        return a.from < b.from;
    }
    return a.to < b.to;
}
}  // namespace

struct BaseMapSaveFilter::Impl {
    //! Owned by caller
    MapData *mapData{};

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
    = default;

void BaseMapSaveFilter::prepare( ProgressCounter *counter )
{
    assert( m_impl->mapData );

    std::set<uint> considered;

    // Seed room
    m_impl->mapData->genericSearch( this, RoomFilter("The Fountain Square", Qt::CaseSensitive,
                                                     PAT_NAME) );

    // Walk the whole map through non-hidden exits without recursing
    while ( !m_impl->roomsTodo.empty() ) {
        uint todo = m_impl->roomsTodo.front();
        m_impl->roomsTodo.pop_front();

        if ( considered.find( todo ) ==
                considered.end() ) { // Don't process twice the same room (ending condition)
            m_impl->mapData->lookingForRooms( this, todo );
            m_impl->mapData->releaseRoom( this, todo );
            considered.insert( todo );
            counter->step();
        }
    }

    for ( uint steps = considered.size(); steps < prepareCount(); ++steps ) {
        counter->step(); // Make up for the secret rooms we skipped
    }
}

void BaseMapSaveFilter::receiveRoom( RoomAdmin * /*admin*/, const Room *room )
{
    for (const auto &exit : room->getExitsList()) {
        for (auto to : exit.outRange()) {
            if (isHiddenExit(exit)) {
                m_impl->secretLinks.emplace(RoomLink(room->getId(), to));
            } else {
                m_impl->baseRooms.emplace(to);
                m_impl->roomsTodo.emplace_back(to);
            }
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
    auto &baseRooms = m_impl->baseRooms;
    assert(!baseRooms.empty() );

    if (baseRooms.find(room->getId()) != baseRooms.end() ) {
        const ExitsList &exits = room->getExitsList();
        for (const auto &exit : exits) {
            if (isHiddenExit(exit)) {
                return ALTER;
            }
            for (auto idx : exit.outRange()) {
                if (contains(baseRooms, idx)) {
                    return ALTER;
                }
            }
            for (auto idx : exit.inRange()) {
                if (contains(baseRooms, idx)) {
                    return ALTER;
                }
            }
        }

        return PASS;
    }
    return REJECT;

}

Room BaseMapSaveFilter::alteredRoom( const Room *room )
{
    auto &baseRooms = m_impl->baseRooms;
    auto &secretLinks = m_impl->secretLinks;
    assert(!baseRooms.empty() );

    Room copy = *room;

    ExitsList &exits = copy.getExitsList();
    for (auto &exit : exits) {

        auto outLinks = exit.outRange();
        auto inLinks = exit.inRange();

        // Destroy links to secret rooms
        for (unsigned int outLink : outLinks) {
            const bool destRoomIsSecret = contains(baseRooms, outLink);
            const bool outLinkIsSecret = (
                                             secretLinks.find(RoomLink(copy.getId(), outLink ) ) != secretLinks.end() );
            const bool linkBackIsSecret = (
                                              secretLinks.find(RoomLink(outLink, copy.getId() ) ) != secretLinks.end() );

            if ( destRoomIsSecret || ( outLinkIsSecret && linkBackIsSecret ) ) {
                exit.removeOut( outLink );

                exit[E_DOORNAME] = "";
                exit[E_FLAGS] = 0;
                exit[E_DOORFLAGS] = 0;

            }
        }

        // Destroy links from secret rooms to here
        for (unsigned int inLink : inLinks) {
            if (contains(baseRooms, inLink)) {
                exit.removeIn( inLink );
            }
        }

        // Remove names on hidden exits to areas reachable through visible doors
        if (isHiddenExit(exit)) {
            exit[E_DOORNAME] = "";
        }
    }

    return copy;
}

