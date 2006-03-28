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

#ifndef PARSEEVENT
#define PARSEEVENT

#include <deque>
#include "property.h"
#include "listcycler.h"
#include "coordinate.h"
#include <QVariant>

/**
 * the ParseEvents will walk around in the SearchTree
 */
class ParseEvent : public ListCycler<Property *, std::deque<Property *> >
{
public:
  ParseEvent(uint move) : moveType(move), numSkipped(0) {}
  ParseEvent(const ParseEvent & other);
  virtual ~ParseEvent();
  ParseEvent & operator=(const ParseEvent & other);

  void reset();
  void countSkipped();
  std::deque<QVariant> & getOptional() {return optional;}
  const std::deque<QVariant> & getOptional() const {return optional;}
  uint getMoveType() const {return moveType;}
  uint getNumSkipped() const {return numSkipped;}
  
private:
  std::deque<QVariant> optional;
  uint moveType;
  uint numSkipped;
  
};

#endif
