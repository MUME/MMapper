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

#ifndef LISTCYCLER
#define LISTCYCLER

#include <climits>
#include <QtGlobal>

template <class T, class C>
class ListCycler : public C {
 public:
  ListCycler() : pos(UINT_MAX) {}
  ListCycler(const C & data) : C(data), pos(data.size()) {}
  virtual ~ListCycler() {}
  virtual T next();
  virtual T prev();
  virtual T current();
  virtual unsigned int getPos() {return pos;}
  virtual void reset() {pos = C::size();}

 protected:
  unsigned int pos;
};


template <class T, class C>
T ListCycler<T,C>::next() {
  const uint nSize = (uint) C::size();
  
  if (pos >= nSize) pos = 0;
  else if (++pos == nSize) return 0;

  if (pos < nSize) return C::operator[](pos);
  else return 0;
}

template <class T, class C>
T ListCycler<T,C>::prev() {
  const uint nSize = (uint) C::size();

  if (pos == 0) {
    pos = nSize;
    return 0;
  }
  else {
    if (pos >= nSize) pos = nSize;
    pos--;
    return C::operator[](pos);
  }
}		

template <class T, class C>	
T ListCycler<T,C>::current() {
  if (pos >= (uint)C::size()) return 0;
  return C::operator[](pos);
}


#ifdef DMALLOC
#include <mpatrol.h>
#endif
#endif

