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

#ifndef ABSTRACTEXIT_H
#define ABSTRACTEXIT_H

#include <set>
#include <QVector>
#include <QVariant>

class Exit : public QVector<QVariant> {
  
  protected:
    std::set<uint> incoming;
    std::set<uint> outgoing;
    
  public:
    Exit(uint numProps = 0) : QVector<QVariant>(numProps) {}
    std::set<uint>::const_iterator inBegin() const {return incoming.begin();}
    std::set<uint>::const_iterator outBegin() const {return outgoing.begin();}
    
    std::set<uint>::const_iterator inEnd() const {return incoming.end();}
    std::set<uint>::const_iterator outEnd() const {return outgoing.end();}
    
    void addIn(uint from) {incoming.insert(from);}
    void addOut(uint to) {outgoing.insert(to);}
    void removeIn(uint from) {incoming.erase(from);}
    void removeOut(uint to) {outgoing.erase(to);}
    bool containsIn(uint from) const {return incoming.find(from) != incoming.end();}
    bool containsOut(uint to) const {return outgoing.find(to) != outgoing.end();}
    void removeAll() {incoming.clear(); outgoing.clear();}
};

#endif
