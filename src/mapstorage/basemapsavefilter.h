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

#ifndef INCLUDED_BASEMAPSAVEFILTER_H
#define INCLUDED_BASEMAPSAVEFILTER_H

#include <memory>

#include "roomrecipient.h"

class MapData;
class Room;
class ProgressCounter;

/*! \brief Filters 
 *
 */
class BaseMapSaveFilter : public RoomRecipient
{
public:
  enum Action
  {
    PASS,
    ALTER,
    REJECT
  };

private:
  class Impl;
  std::auto_ptr<Impl> m_impl;

  // Disallow copying because auto_ptr is used
  BaseMapSaveFilter( const BaseMapSaveFilter & );
  BaseMapSaveFilter & operator=( const BaseMapSaveFilter & );

  virtual void receiveRoom(RoomAdmin *, const Room * room);

public:
  BaseMapSaveFilter();
  virtual ~BaseMapSaveFilter();

  //! The map data to work on
  void setMapData( MapData *mapData );
  //! How much steps (rooms) to go through in prepare() (requires mapData)
  uint prepareCount();
  //! How much rooms will be accepted (PASS or ALTER)
  uint acceptedRoomsCount();
  //! First pass over the world's room (requires mapData)
  void prepare( ProgressCounter *counter );
  //! Determines the fate of this room (requires prepare())
  Action filter( const Room *room );
  //! Returns an altered Room (requires action == ALTER)
  Room alteredRoom( const Room *room );
};

#endif /* INCLUDED_BASEMAPSAVEFILTER_H */
