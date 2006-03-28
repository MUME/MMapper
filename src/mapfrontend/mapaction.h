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

#ifndef MAPACTION_H
#define MAPACTION_H
#include "roomselection.h"
#include <set>
#include "map.h"
#include "intermediatenode.h"
#include "mapfrontend.h"


class FrontendAccessor {
  public:
    virtual ~FrontendAccessor() {}
    virtual void setFrontend(MapFrontend * in) {frontend = in;}
  protected:
    MapFrontend * frontend;
    Map & map() {return frontend->map;}
    IntermediateNode & treeRoot() {return frontend->treeRoot;}
    std::vector<Room *> & roomIndex() {return frontend->roomIndex;}
    std::vector<RoomCollection *> & roomHomes() {return frontend->roomHomes;}
    std::stack<uint> & unusedIds() {return frontend->unusedIds;}
    AbstractRoomFactory * factory() {return frontend->factory;}
};

class AbstractAction : public FrontendAccessor {
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

class Update : public AbstractAction {
  public:
    Update(ParseEvent * props);
    virtual void exec(uint id);
  protected:
    ParseEvent props;
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
