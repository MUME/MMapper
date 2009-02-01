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

#ifndef MMAPPER2EXIT_H
#define MMAPPER2EXIT_H
#include "defs.h"
#include "parser.h"
#include "exit.h"

class Room;

enum ExitType { ET_NORMAL, ET_LOOP, ET_ONEWAY, ET_UNDEFINED };

enum ExitDirection { ED_NORTH=0, ED_SOUTH, ED_EAST, ED_WEST, ED_UP, 
			   ED_DOWN, ED_UNKNOWN, ED_NONE };

ExitDirection opposite(ExitDirection in);
uint opposite(uint in);
ExitDirection dirForChar(char dir);

typedef class QString DoorName;

#define EF_EXIT       bit1
#define EF_DOOR       bit2
#define EF_ROAD       bit3
#define EF_CLIMB      bit4
#define EF_RANDOM     bit5
#define EF_SPECIAL    bit6
#define EF_NO_MATCH   bit7

#define DF_HIDDEN     bit1
#define DF_NEEDKEY    bit2
#define DF_NOBLOCK    bit3
#define DF_NOBREAK    bit4
#define DF_NOPICK     bit5
#define DF_DELAYED    bit6
#define DF_RESERVED1  bit7
#define DF_RESERVED2  bit8

enum ExitField {E_DOORNAME = 0, E_FLAGS, E_DOORFLAGS};

typedef quint8 ExitFlags;
typedef quint8 DoorFlags;

ExitFlags getFlags(const Exit & e);

DoorName getDoorName(const Exit & e);

DoorFlags getDoorFlags(const Exit & e);

void updateExit(Exit & e, ExitFlags flags);

void orExitFlags(Exit & e, ExitFlags flags);
  
void nandExitFlags(Exit & e, ExitFlags flags);

void orDoorFlags(Exit & e, DoorFlags flags);
  
void nandDoorFlags(Exit & e, DoorFlags flags);
  
#endif
