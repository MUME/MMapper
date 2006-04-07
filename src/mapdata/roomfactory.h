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

#ifndef ROOMFACTORY
#define ROOMFACTORY

#include "abstractroomfactory.h"
#include <QVector>

class RoomFactory : public AbstractRoomFactory
{
public:
  RoomFactory();
  virtual Room * createRoom(const ParseEvent * event = 0) const;
  virtual ComparisonResult compare(const Room *, const ParseEvent * props, uint tolerance = 0) const;
  virtual ComparisonResult compareWeakProps(const Room *, const ParseEvent * props, uint tolerance = 0) const;
  virtual ParseEvent * getEvent(const Room *) const;
  virtual void update(Room *, const ParseEvent * event) const;
  virtual void update(Room * target, const Room * source) const;
  virtual uint opposite(uint dir) const;
  virtual const Coordinate & exitDir(uint dir) const;
  virtual uint numKnownDirs() const {return 8;}
  virtual ~RoomFactory() {}
private:
  QVector<Coordinate> exitDirs;
  ComparisonResult compareStrings(const QString & room, const QString & event, uint tolerance, bool updated = true) const;
};

#endif
