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

#ifndef LISTCYCLER
#define LISTCYCLER

#include <climits>

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

