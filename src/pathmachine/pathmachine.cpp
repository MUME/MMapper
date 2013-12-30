/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "pathmachine.h"
#include "configuration.h"
#include "approved.h"
#include "path.h"
#include "abstractroomfactory.h"
#include "crossover.h"
#include "syncing.h"
#include "onebyone.h"
#include "parseevent.h"
#include "customaction.h"

#include <stack>

using namespace std;
using namespace Qt;

PathMachine::PathMachine(AbstractRoomFactory * in_factory, bool threaded) :
    Component(threaded),
    factory(in_factory),
    signaler(this),
    pathRoot(0,0,0),
    mostLikelyRoom(0,0,0),
    lastEvent(0),
    state(SYNCING),
    paths(new list<Path *>)
{}

void PathMachine::setCurrentRoom(Approved * app)
{
  releaseAllPaths();
  const Room * perhaps = app->oneMatch();
  if (perhaps)
  {
    mostLikelyRoom = *perhaps;
    emit playerMoved(mostLikelyRoom.getPosition());
    state = APPROVED;
  }
}

void PathMachine::setCurrentRoom(const Coordinate & pos)
{
  Approved app(factory, lastEvent, 100);
  emit lookingForRooms(&app, pos);
  setCurrentRoom(&app);
}

void PathMachine::setCurrentRoom(uint id)
{
  Approved app(factory, lastEvent, 100);
  emit lookingForRooms(&app, id);
  setCurrentRoom(&app);
}


void PathMachine::init()
{
  QObject::connect(&signaler, SIGNAL(scheduleAction(MapAction *)), this, SIGNAL(scheduleAction(MapAction *)));
}

/**
 * the signals - apart from playerMoved have to be DirectConnections because we want to
 * be sure to find all available rooms and we also want to make sure a room is actually
 * inserted into the map before we search for it
 * The slots need to be queued because we want to make sure all data is only accessed
 * from this thread
 */
ConnectionType PathMachine::requiredConnectionType(const QString & str)
{

  if (str == SLOT(event(ParseEvent *)) ||
      str == SLOT(deleteMostLikelyRoom()))
    return QueuedConnection;
  else if (str == SIGNAL(playerMoved(Coordinate, Coordinate)))
    return AutoCompatConnection;
  else
    return DirectConnection;
}

void PathMachine::releaseAllPaths()
{
  for (list<Path*>::iterator i = paths->begin(); i != paths->end(); ++i)
    (*i)->deny();
  paths->clear();

  state = SYNCING;
}

void PathMachine::retry()
{

  switch (state)
  {
  case APPROVED:
    state = SYNCING;
    break;
  case EXPERIMENTING:
    releaseAllPaths();
    break;
  }
  if (lastEvent) event(lastEvent);
}

void PathMachine::event(ParseEvent * ev)
{
  if (ev != lastEvent)
  {
    delete lastEvent;
    lastEvent = ev;
  }
  switch (state)
  {
  case APPROVED:
    approved(ev);
    break;
  case EXPERIMENTING:
    experimenting(ev);
    break;
  case SYNCING:
    syncing(ev);
    break;
  }
}

void PathMachine::deleteMostLikelyRoom()
{
  if (state == EXPERIMENTING)
  {
    list<Path *> * newPaths = new list<Path *>;
    Path * best = 0;
    for (list<Path*>::iterator i = paths->begin(); i != paths->end(); ++i)
    {
      Path * working = *i;
      if ((working)->getRoom()->getId() == mostLikelyRoom.getId()) working->deny();
      else if (best == 0) best = working;
      else if (working->getProb() > best->getProb())
      {
        newPaths->push_back(best);
        best = working;
      }
      else newPaths->push_back(working);
    }
    if (best) newPaths->push_front(best);
    delete paths;
    paths = newPaths;

  }
  else
  {
    paths->clear(); // throw the parser into syncing
  }
  evaluatePaths();
}



void PathMachine::tryExits(const Room * room, RoomRecipient * recipient, ParseEvent * event, bool out)
{
  uint move = event->getMoveType();
  if (move < (uint)room->getExitsList().size())
  {
    const Exit & possible = room->exit(move);
    tryExit(possible, recipient, out);
  }
  else
  {
    emit lookingForRooms(recipient, room->getId());
    if (move >= factory->numKnownDirs())
    {
      const ExitsList & eList = room->getExitsList();
      for (int listit = 0; listit < eList.size(); ++listit)
      {
        const Exit & possible = eList[listit];
        tryExit(possible, recipient, out);
      }
    }
  }
}

void PathMachine::tryExit(const Exit & possible, RoomRecipient * recipient, bool out)
{
  set<uint>::const_iterator begin;
  set<uint>::const_iterator end;
  if (out)
  {
    begin = possible.outBegin();
    end = possible.outEnd();
  }
  else
  {
    begin = possible.inBegin();
    end = possible.inEnd();
  }
  for (set<uint>::const_iterator i = begin; i != end; ++i)
  {
    emit lookingForRooms(recipient, *i);
  }
}


void PathMachine::tryCoordinate(const Room * room, RoomRecipient * recipient, ParseEvent * event)
{
  uint moveCode = event->getMoveType();
  uint size = factory->numKnownDirs();
  if (size > moveCode)
  {
    Coordinate c = room->getPosition() + factory->exitDir(moveCode);
    emit lookingForRooms(recipient, c);
  }
  else
  {
    Coordinate roomPos = room->getPosition();
    for (uint i = 0; i < size; ++i)
    {
      emit lookingForRooms(recipient, roomPos + factory->exitDir(i));
    }
  }
}

void PathMachine::approved(ParseEvent * event)
{
  Approved appr(factory, event, params.matchingTolerance);
  const Room * perhaps = 0;

  tryExits(&mostLikelyRoom, &appr, event, true);

  perhaps = appr.oneMatch();

  if (!perhaps)
  { // try to match by reverse exit
    appr.reset();
    tryExits(&mostLikelyRoom, &appr, event, false);
    perhaps = appr.oneMatch();
    if (!perhaps)
    { // try to match by coordinate
      appr.reset();
      tryCoordinate(&mostLikelyRoom, &appr, event);
      perhaps = appr.oneMatch();
      if (!perhaps)
      { // try to match by coordinate one step below expected
        const Coordinate & eDir = factory->exitDir(event->getMoveType());
        if (eDir.z == 0)
        {
          appr.reset();
          Coordinate c = mostLikelyRoom.getPosition() + eDir;
          c.z--;
          emit lookingForRooms(&appr, c);
          perhaps = appr.oneMatch();

          if (!perhaps)
          { // try to match by coordinate one step above expected
            appr.reset();
            c.z += 2;
            emit lookingForRooms(&appr, c);
            perhaps = appr.oneMatch();
          }
        }
      }
    }
  }
  if (perhaps)
  {
    uint move = event->getMoveType();
    if ((uint)mostLikelyRoom.getExitsList().size() > move)
    {
      emit scheduleAction(new AddExit(mostLikelyRoom.getId(), perhaps->getId(), move));
    }
    mostLikelyRoom = *perhaps;
    emit playerMoved(mostLikelyRoom.getPosition());
    if (Config().m_groupManagerState != 2) emit setCharPosition(mostLikelyRoom.getId()); // GroupManager
  }
  else
  { // couldn't match, give up
    state = EXPERIMENTING;
    pathRoot = mostLikelyRoom;
    Path * root = new Path(&pathRoot, 0, 0, &signaler);
    paths->push_front(root);
    experimenting(event);
  }
}


void PathMachine::syncing(ParseEvent * event)
{
  {
    Syncing sync(params, paths, &signaler);
    if (event->getNumSkipped() <= params.maxSkipped)
      emit lookingForRooms(&sync, event);
    paths = sync.evaluate();
  }
  evaluatePaths();
}


void PathMachine::experimenting(ParseEvent * event)
{
  Experimenting * exp = 0;
  uint moveCode = event->getMoveType();
  Coordinate move = factory->exitDir(moveCode);
  // only create rooms if no properties are skipped and
  // the move coordinate is not 0,0,0
  if (event->getNumSkipped() == 0 && (moveCode < (uint)mostLikelyRoom.getExitsList().size()))
  {
    exp = new Crossover(paths, moveCode, params, factory);
    set<const Room *> pathEnds;
    for (list<Path *>::iterator i = paths->begin(); i != paths->end(); ++i)
    {
      const Room * working = (*i)->getRoom();
      if (pathEnds.find(working) == pathEnds.end())
      {
        emit createRoom(event, working->getPosition() + move);
        pathEnds.insert(working);
      }
    }
    emit lookingForRooms(exp, event);
  }
  else
  {
    OneByOne * oneByOne = new OneByOne(factory, event, params, &signaler);
    exp = oneByOne;
    for (list<Path *>::iterator i = paths->begin(); i != paths->end(); ++i)
    {
      const Room * working = (*i)->getRoom();
      oneByOne->addPath(*i);
      tryExits(working, exp, event, true);
      tryExits(working, exp, event, false);
      tryCoordinate(working, exp, event);
    }
  }

  paths = exp->evaluate();
  delete exp;
  evaluatePaths();
}

void PathMachine::evaluatePaths()
{
  if (paths->empty())
  {
    state = SYNCING;
  }
  else
  {
    mostLikelyRoom = *(paths->front()->getRoom());
    if (++paths->begin() == paths->end())
    {
      state = APPROVED;
      paths->front()->approve();
      paths->pop_front();
    }
    else
    {
      state = EXPERIMENTING;
    }
    emit playerMoved(mostLikelyRoom.getPosition());
  }
}





