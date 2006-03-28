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

#ifndef TINYLIST
#define TINYLIST
/**
 * extremely shrinked array list implementation to be used for each room
 * because we have so many rooms and want to search them really fast, we
 *	- allocate as little memory as possible
 *	- allow only 128 elements (1 per character)
 *	- only need 3 lines to access an element
 */

template <class T>
class TinyList
{
public:
  TinyList() : list(0), listSize(0) {}

  virtual ~TinyList()
  {
    if (list) delete[] list;
  }

  virtual T get(unsigned char c)
  {
    if (c >= listSize) return 0;
    else return list[c];
  }
  
  virtual void put(unsigned char c, T object)
  {
    if (c >= listSize)
    {
      unsigned char i;
      T * nlist = new T[c+2];
      for (i = 0; i < listSize; i++) nlist[i] = list[i];
      for (; i < c; i++) nlist[i] = 0;
      if (list) delete[] list;
      list = nlist;
      listSize = c+1;
      list[listSize] = 0;
    }
    list[c] = object;
  }
  
  virtual void remove(unsigned char c)
  {
    if (c < listSize) list[c] = 0;
  }
  
  virtual unsigned char size()
  {
    return listSize;
  }

protected:
  T * list;
  unsigned char listSize;
};






#ifdef DMALLOC
#include <mpatrol.h>
#endif
#endif
