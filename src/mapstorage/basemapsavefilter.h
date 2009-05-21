/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
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

#ifndef INCLUDED_BASEMAPSAVEFILTER_H
#define INCLUDED_BASEMAPSAVEFILTER_H

#include <memory>

#include "expandoracommon/roomrecipient.h"

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
