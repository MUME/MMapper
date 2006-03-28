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

#include "parseevent.h"

using namespace std;


ParseEvent::ParseEvent(const ParseEvent & other) : ListCycler<Property *, deque<Property *> >(), optional(other.optional), moveType(other.moveType), numSkipped(other.numSkipped) {
  for (unsigned int i = 0; i < other.size(); ++i)
    push_back(new Property(*other[i]));
  pos = other.pos;
  
}
  
ParseEvent & ParseEvent::operator=(const ParseEvent & other) {
  if (&other == this) return *this;
  moveType = other.moveType;
  pos = other.pos;
  optional = other.optional;
  numSkipped = other.numSkipped;
  for (deque<Property *>::iterator i = begin(); i < end(); ++i)
    delete *i;
  for (unsigned int i = 0; i < other.size(); ++i)
    push_back(new Property(*other[i]));
  return *this;
}

ParseEvent::~ParseEvent() {
  for (deque<Property *>::iterator i = begin(); i < end(); ++i)
    delete *i;
}

void ParseEvent::reset() {
  ListCycler<Property *, deque<Property *> >::reset();
  for (deque<Property *>::iterator i = begin(); i < end(); ++i)
    (*i)->reset();
}

void ParseEvent::countSkipped() {
  numSkipped = 0;
  for (deque<Property *>::iterator i = begin(); i != end(); ++i) {
    if ((*i)->isSkipped()) numSkipped++;
  }
}

