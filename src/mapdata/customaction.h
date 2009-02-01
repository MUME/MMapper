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

#ifndef CUSTOMACTION_H
#define CUSTOMACTION_H
#include "mapaction.h"
#include <list>

enum FlagModifyMode {FMM_SET, FMM_UNSET, FMM_TOGGLE};

class MapData;

typedef AddExit AddOneWayExit;

class AddTwoWayExit : public AddOneWayExit {
	public:
		AddTwoWayExit(uint room1Id, uint room2Id, uint room1Dir, uint in_room2Dir = UINT_MAX) :
			AddOneWayExit(room1Id, room2Id, room1Dir), room2Dir(in_room2Dir) {}
	protected:
		virtual void exec();
		uint room2Dir;
};

typedef RemoveExit RemoveOneWayExit;

class RemoveTwoWayExit : public RemoveOneWayExit {
	public:
		RemoveTwoWayExit(uint room1Id, uint room2Id, uint room1Dir, uint in_room2Dir = UINT_MAX) :
			RemoveOneWayExit(room1Id, room2Id, room1Dir), room2Dir(in_room2Dir) {}
	protected:
		virtual void exec();
		uint room2Dir;
};


class GroupAction : virtual public MapAction
{
  public: 
    GroupAction(AbstractAction * ex, const RoomSelection * selection);
    void schedule(MapFrontend * in) {executor->setFrontend(in);}
    virtual ~GroupAction() {delete executor;}
  protected:
    virtual void exec();
    virtual const std::set<uint> & getAffectedRooms();
  private:
    std::list<uint> selectedRooms;
    AbstractAction * executor;
};

class MoveRelative : public AbstractAction
{
public:
  MoveRelative(const Coordinate & move);
  virtual void preExec(uint id);
  virtual void exec(uint id);
protected:
  Coordinate move;
};

class MergeRelative : public Remove
{
public:
  MergeRelative(const Coordinate & move);
  virtual void preExec(uint id);
  virtual void exec(uint id);
  virtual void insertAffected(uint id, std::set<uint> & affected);
protected:
  Coordinate move;
};


class ConnectToNeighbours : public AbstractAction
{
public:
  virtual void insertAffected(uint id, std::set<uint> & affected);
  virtual void exec(uint id);
private:
  void connectRooms(Room * center, Coordinate & otherPos, uint dir, uint cid);
};

class DisconnectFromNeighbours :  public ExitsAffecter
{
public:
  virtual void exec(uint id);
};



class ModifyRoomFlags : public AbstractAction
{
public:
  ModifyRoomFlags(uint flags, uint fieldNum, FlagModifyMode);
  virtual void exec(uint id);
protected:
  const uint flags;
  const uint fieldNum;
  const FlagModifyMode mode;
};

class UpdateExitField : public AbstractAction
{
public:
  UpdateExitField(const QVariant & update, uint dir, uint fieldNum);
  virtual void exec(uint id);
protected:
  const QVariant update;
  const uint fieldNum;
  const uint dir;
};

class ModifyExitFlags : public AbstractAction
{
public:
  ModifyExitFlags(uint flags, uint dir, uint exitField, FlagModifyMode);
  virtual void exec(uint id);
protected:
  const uint flags;
  const uint fieldNum;
  const FlagModifyMode mode;
  const uint dir;
};

#endif
