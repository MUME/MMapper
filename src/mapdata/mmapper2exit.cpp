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

#include "mmapper2exit.h"


ExitFlags getFlags(const Exit & e) 
  {return e[E_FLAGS].toUInt();}

DoorName getDoorName(const Exit & e)
  {return e[E_DOORNAME].toString();}

DoorFlags getDoorFlags(const Exit & e)
  {return e[E_DOORFLAGS].toUInt();}

void orExitFlags(Exit & e, ExitFlags flags)
  {e[E_FLAGS] = getFlags(e) | flags;}
  
void nandExitFlags(Exit & e, ExitFlags flags)
  {e[E_FLAGS] = getFlags(e) & ~flags;}

void orDoorFlags(Exit & e, DoorFlags flags)
  {e[E_DOORFLAGS] = getDoorFlags(e) | flags;}
  
void nandDoorFlags(Exit & e, DoorFlags flags)
  {e[E_DOORFLAGS] = getDoorFlags(e) & ~flags;}


uint opposite(uint in) {
  return opposite((ExitDirection)in);
}

ExitDirection opposite(ExitDirection in)
{
  switch (in)
  {
  case ED_NORTH: return ED_SOUTH;
  case ED_SOUTH: return ED_NORTH;
  case ED_WEST: return ED_EAST;
  case ED_EAST: return ED_WEST;
  case ED_UP: return ED_DOWN;
  case ED_DOWN: return ED_UP;
  default: return ED_UNKNOWN;
  }
}
 

void updateExit(Exit & e, ExitFlags flags ) {
  ExitFlags diff = flags ^ getFlags(e);
  if (diff) {
    flags |= EF_NO_MATCH;
    flags |= diff;
    e[E_FLAGS] = flags;
  }
}


ExitDirection dirForChar(char dir) {
	switch (dir) {
		case 'n': return ED_NORTH;
		case 's': return ED_SOUTH;
		case 'e': return ED_EAST;
		case 'w': return ED_WEST;
		case 'u': return ED_UP;
		case 'd': return ED_DOWN;
		default: return ED_UNKNOWN;
	}
}

