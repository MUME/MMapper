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

#ifndef PARSEEVENT
#define PARSEEVENT

#include <deque>
#include "listcycler.h"
#include <QVariant>
#include <QMutex>

class Property;

/**
 * the ParseEvents will walk around in the SearchTree
 */
class ParseEvent : public ListCycler<Property *, std::deque<Property *> >
{
public:
  ParseEvent(uint move) : moveType(move), numSkipped(0) {parsedMutex.lock();}
  ParseEvent(const ParseEvent & other);
  virtual ~ParseEvent();
  ParseEvent & operator=(const ParseEvent & other);

  void reset();
  void countSkipped();
  std::deque<QVariant> & getOptional() {return optional;}
  const std::deque<QVariant> & getOptional() const {return optional;}
  uint getMoveType() const {return moveType;}
  uint getNumSkipped() const {return numSkipped;}

  void setParsed();
  bool waitForParsed(int timeout=-1);

private:
  std::deque<QVariant> optional;
  uint moveType;
  uint numSkipped;
  QMutex parsedMutex;

};

#endif
