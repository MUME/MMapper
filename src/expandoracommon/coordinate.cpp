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

#include "coordinate.h"
using namespace std;

bool Coordinate::operator== (const Coordinate & other) const {
  return (other.x == x && other.y == y && other.z == z);
}

bool Coordinate::operator!= (const Coordinate & other) const {
  return (other.x != x || other.y != y || other.z != z);
}

int Coordinate::distance(const Coordinate & other) const {
  int ret = abs(x - other.x);
  ret += abs(y - other.y);
  ret += abs(z - other.z);
  return ret;
}

void Coordinate::clear() {
  x = 0;
  y = 0;
  z = 0;
}

void Coordinate::operator+=(const Coordinate & other) {
  x += other.x;
  y += other.y;
  z += other.z;
}

void Coordinate::operator-=(const Coordinate & other) {
  x -= other.x;
  y -= other.y;
  z -= other.z;
}

Coordinate Coordinate::operator+(const Coordinate & other) const {
  Coordinate ret;
  ret += *this;
  ret += other;
  return ret;
}

Coordinate Coordinate::operator-(const Coordinate & other) const {
  Coordinate ret;
  ret += *this;
  ret -= other;
  return ret;
}
