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

#ifndef ROOM_H
#define ROOM_H

#include <QList>
#include <QVariant>
#include <QVector>
#include "coordinate.h"
#include "parseevent.h"
#include "exit.h"


typedef QVector<Exit> ExitsList;
typedef QVectorIterator<Exit> ExitsListIterator;

class Room : public QVector<QVariant>
{

protected:
  uint id;
  Coordinate position;
  bool temporary;
  bool upToDate;
  ExitsList exits;

public:
  virtual Exit & exit(uint dir){return exits[dir];}
  ExitsList & getExitsList(){return exits;}
  const ExitsList & getExitsList() const {return exits;}
  virtual ~Room() {}
  virtual void setId(uint in) {id = in;}
  virtual void setPosition(const Coordinate & in_c) {position = in_c;}
  virtual uint getId() const {return id;}
  virtual const Coordinate & getPosition() const {return position;}
  virtual bool isTemporary() const {return temporary;} // room is new if no exits are present
  virtual void setPermanent() {temporary = false;}
  virtual bool isUpToDate() const {return upToDate;}
  virtual const Exit & exit(uint dir) const {return exits[dir];}  
  virtual void setUpToDate() {upToDate = true;}
  virtual void setOutDated() {upToDate = false;}
  Room(uint numProps, uint numExits, uint numExitProps) : 
    QVector<QVariant>(numProps), 
    id(UINT_MAX),
    temporary(true),
    upToDate(false),
    exits(numExits, numExitProps)
  {}
};


#endif
