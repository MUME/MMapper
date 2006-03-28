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

#ifndef COORDINATE
#define COORDINATE
#include <vector>
#include <map>
#include <qstring.h>




class Coordinate
{
public:
  bool operator== (const Coordinate & other) const;
  bool operator!= (const Coordinate & other) const;
  void operator+= (const Coordinate & other);
  void operator-= (const Coordinate & other);
  Coordinate operator+ (const Coordinate & other) const;
  Coordinate operator- (const Coordinate & other) const;

  int distance(const Coordinate & other) const;
  void clear();
  Coordinate(int in_x = 0, int in_y = 0, int in_z = 0) : x(in_x), y(in_y), z(in_z) {}
  bool isNull() const {return (x == 0 && y == 0 && z == 0);}

  int x;
  int y;
  int z;
};



#ifdef DMALLOC
#include <mpatrol.h>
#endif
#endif


