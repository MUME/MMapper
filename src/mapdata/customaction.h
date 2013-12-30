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

#ifndef CUSTOMACTION_H
#define CUSTOMACTION_H
#include "mapaction.h"
#include "coordinate.h"
#include <list>

enum FlagModifyMode {FMM_SET, FMM_UNSET, FMM_TOGGLE};

class MapData;
class ParseEvent;
class RoomSelection;

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
