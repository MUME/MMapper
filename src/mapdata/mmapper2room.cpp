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

#include "mmapper2room.h"

RoomName getName(const Room * room)
  { return (*room)[R_NAME].toString(); }
  
RoomDescription getDescription(const Room * room)
  { return (*room)[R_DESC].toString(); }
  
RoomDescription getDynamicDescription(const Room * room) 
  { return (*room)[R_DYNAMICDESC].toString(); }
  
RoomNote getNote(const Room * room) 
  { return (*room)[R_NOTE].toString(); }

RoomMobFlags getMobFlags(const Room * room) 
  { return (*room)[R_MOBFLAGS].toUInt(); }
  
RoomLoadFlags getLoadFlags(const Room * room) 
  { return (*room)[R_LOADFLAGS].toUInt(); }
  
RoomTerrainType getTerrainType(const Room * room) 
  { return (RoomTerrainType)(*room)[R_TERRAINTYPE].toUInt(); }
  
RoomPortableType getPortableType(const Room * room) 
  { return (RoomPortableType)(*room)[R_PORTABLETYPE].toUInt(); }
  
RoomLightType getLightType(const Room * room) 
  { return (RoomLightType)(*room)[R_LIGHTTYPE].toUInt(); }
  
RoomAlignType getAlignType(const Room * room) 
  { return (RoomAlignType)(*room)[R_ALIGNTYPE].toUInt(); }

RoomRidableType getRidableType(const Room * room) 
  { return (RoomRidableType)(*room)[R_RIDABLETYPE].toUInt(); }

