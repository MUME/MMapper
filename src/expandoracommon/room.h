/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef ROOM_H
#define ROOM_H

#include <QVariant>
#include <QVector>
#include "coordinate.h"
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
