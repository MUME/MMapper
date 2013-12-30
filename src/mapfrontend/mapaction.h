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

#ifndef MAPACTION_H
#define MAPACTION_H
//#include "roomselection.h"
//#include "map.h"
//#include "intermediatenode.h"
//#include "mapfrontend.h"
#include "parseevent.h"

#include <QtGlobal>
#include <QVariant>

#include <set>
#include <vector>
#include <stack>

class Map;
class MapFrontend;
class AbstractRoomFactory;
class IntermediateNode;
class RoomCollection;
class Room;

class FrontendAccessor {
  public:
    virtual ~FrontendAccessor() {}
    virtual void setFrontend(MapFrontend * in) ;
  protected:
    MapFrontend * m_frontend;
    Map & map() ;
    IntermediateNode & treeRoot() ;
    std::vector<Room *> & roomIndex() ;
    std::vector<RoomCollection *> & roomHomes() ;
    std::stack<uint> & unusedIds() ;
    AbstractRoomFactory * factory() ;
};

class AbstractAction : public virtual FrontendAccessor {
  public:
    virtual void preExec(uint) {}
    virtual void exec(uint id) = 0;
    virtual void insertAffected(uint id, std::set<uint> & affected)
      {affected.insert(id);}
};

class MapAction {
  friend class MapFrontend;
  public:
    virtual void schedule(MapFrontend * in) = 0;
    virtual ~MapAction() {}
  protected:
    virtual void exec() = 0;
    virtual const std::set<uint> & getAffectedRooms() 
      {return affectedRooms;}
    std::set<uint> affectedRooms;
};

class SingleRoomAction : public MapAction {
  public: 
    SingleRoomAction(AbstractAction * ex, uint id);
    void schedule(MapFrontend * in) {executor->setFrontend(in);}
    virtual ~SingleRoomAction() {delete executor;}
  protected:
    virtual void exec() {executor->preExec(id); executor->exec(id);}
    virtual const std::set<uint> & getAffectedRooms();
  private:
    uint id;
    AbstractAction * executor;
};

class AddExit: public MapAction, public FrontendAccessor {
  public:
    void schedule(MapFrontend * in) {setFrontend(in);}
    AddExit(uint in_from, uint in_to, uint in_dir);
  protected:
    virtual void exec();
		Room * tryExec();
    uint from;
    uint to;
    uint dir;
};

class RemoveExit: public MapAction, public FrontendAccessor {
  public:
    RemoveExit(uint from, uint to, uint dir);
    void schedule(MapFrontend * in) {setFrontend(in);}
  protected:
    virtual void exec();
		Room * tryExec();
    uint from;
    uint to;
    uint dir;
};

class MakePermanent: public AbstractAction {
  public:
    virtual void exec(uint id);
};

class UpdateRoomField : public virtual AbstractAction
{
public:
  UpdateRoomField(const QVariant & update, uint fieldNum);
  virtual void exec(uint id);
protected:
  const QVariant update;
  const uint fieldNum;
};

class Update : public virtual AbstractAction {
  public:
    Update(ParseEvent * props);
    virtual void exec(uint id);
  protected:
    ParseEvent props;
};

class UpdatePartial :  public virtual Update,  public virtual UpdateRoomField {
	public:
		UpdatePartial(const QVariant & in_val, uint in_pos);
		virtual void exec(uint id);
};

class ExitsAffecter : public AbstractAction {
  public:
    virtual void insertAffected(uint id, std::set<uint> & affected);
};

class Remove : public ExitsAffecter {
  protected:
    virtual void exec(uint id);
};


#endif
