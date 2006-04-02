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

#include "mmapper2pathmachine.h"
#include "mmapper2event.h"
#include "roomfactory.h"

#ifdef MODULAR
extern "C" MY_EXPORT Component * createComponent()
{
  return new Mmapper2PathMachine;
}
#else
Initializer<Mmapper2PathMachine> mmapper2PathMachine("Mmapper2PathMachine");
#endif

void Mmapper2PathMachine::event(ParseEvent * event)
{
  params.acceptBestRelative = config.m_acceptBestRelative;
  params.acceptBestAbsolute = config.m_acceptBestAbsolute;
  params.newRoomPenalty = config.m_newRoomPenalty;
  params.correctPositionBonus = config.m_correctPositionBonus;
  params.maxPaths = config.m_maxPaths;
  params.matchingTolerance = config.m_matchingTolerance;
	params.multipleConnectionsPenalty = config.m_multipleConnectionsPenalty;
	
  QString stringState = "received event, state: ";
  if (state == EXPERIMENTING) stringState += "EXPERIMENTING";
  else if (state == SYNCING) stringState += "SYNCING";
  else stringState += "APPROVED";
  QString me("PathMachine");
  emit log(me, stringState);
  PathMachine::event(event);
  stringState = "done processing event, state: ";
  if (state == EXPERIMENTING) stringState += "EXPERIMENTING";
  else if (state == SYNCING) stringState += "SYNCING";
  else stringState += "APPROVED";
  emit log(me, stringState);
}

Mmapper2PathMachine::Mmapper2PathMachine() : PathMachine(new RoomFactory, false), config(Config())
{}

