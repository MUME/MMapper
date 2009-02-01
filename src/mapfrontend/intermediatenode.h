/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef INTERMEDIATENODE
#define INTERMEDIATENODE

#include "searchtreenode.h"
#include "roomcollection.h"
#include "roomoutstream.h"


/**
 * IntermediateNodes represent possible ends of a property
 * they hold a RoomSearchNode if this property can be the last one
 */
class IntermediateNode : public SearchTreeNode {
 public:
  virtual ~IntermediateNode() {delete rooms;}
  IntermediateNode() : SearchTreeNode((char*)"") {rooms = 0;}
  IntermediateNode(ParseEvent * event);
  RoomCollection * insertRoom(ParseEvent * event); 
  void getRooms(RoomOutStream & stream, ParseEvent * event);
  void skipDown(RoomOutStream & stream, ParseEvent * event);
 private:
  RoomCollection * rooms;
};

#endif

#ifdef DMALLOC
#include <mpatrol.h>
#endif
