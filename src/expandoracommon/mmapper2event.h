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

#ifndef MMAPPER2EVENT_H
#define MMAPPER2EVENT_H

#include <QString>
#include "parser.h"
#include "parseevent.h"

#define EV_NAME 0
#define EV_DESC 1
#define EV_PDESC 2
#define EV_EXITS 3
#define EV_PROMPT 4

ParseEvent * createEvent(const CommandIdType & c, const QString & roomName, const QString & roomDesc, const QString & parsedRoomDesc, const ExitsFlagsType & exitFlags, const PromptFlagsType & promptFlags);

QString getRoomName(const ParseEvent * e);

QString getRoomDesc(const ParseEvent * e);
  
QString getParsedRoomDesc(const ParseEvent * e);
  
ExitsFlagsType getExitFlags(const ParseEvent * e);
  
PromptFlagsType getPromptFlags(const ParseEvent * e);
  

#endif
