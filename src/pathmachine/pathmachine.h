#pragma once
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

#ifndef PARSER
#define PARSER

#include <list>
#include <memory>
#include <QString>
#include <QtCore>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

class AbstractRoomFactory;
class Approved;
class Coordinate;
class Exit;
class MapAction;
class Path;
class QEvent;
class QObject;
class RoomRecipient;
struct RoomId;

enum class PathState { APPROVED = 0, EXPERIMENTING = 1, SYNCING = 2 };

/**
 * the parser determines the relations between incoming move- and room-events
 * and decides if rooms have to be added (and where) and where the player is
 * the results are published via signals
 */
class PathMachine : public QObject
{
    Q_OBJECT
public slots:
    // CAUTION: This hides virtual bool QObject::event(QEvent*).
    virtual void event(const SigParseEvent &);
    virtual void releaseAllPaths();
    virtual void setCurrentRoom(RoomId id, bool update);

signals:
    void lookingForRooms(RoomRecipient &, const SigParseEvent &);
    void lookingForRooms(RoomRecipient &, RoomId);
    void lookingForRooms(RoomRecipient &, const Coordinate &);
    void playerMoved(const Coordinate &);
    void createRoom(const SigParseEvent &, const Coordinate &);
    void scheduleAction(MapAction *);
    void setCharPosition(RoomId id);

public:
    explicit PathMachine(AbstractRoomFactory *factory, QObject *parent = nullptr);

protected:
    PathParameters params{};
    void experimenting(const SigParseEvent &sigParseEvent);
    void syncing(const SigParseEvent &sigParseEvent);
    void approved(const SigParseEvent &sigParseEvent);
    void evaluatePaths();
    void tryExits(const Room *, RoomRecipient &, ParseEvent &, bool out);
    void tryExit(const Exit &possible, RoomRecipient &recipient, bool out);
    void tryCoordinate(const Room *, RoomRecipient &, ParseEvent &);
    AbstractRoomFactory *factory;

    RoomSignalHandler signaler;
    /* REVISIT: pathRoot and mostLikelyRoom should probably be of type RoomId */
    Room pathRoot;
    Room mostLikelyRoom;
    SigParseEvent lastEvent{};
    PathState state = PathState::SYNCING;
    std::list<Path *> *paths = nullptr;

private:
    // avoid warning about signal hiding this function
    virtual bool event(QEvent *e) final override { return QObject::event(e); }
};

#endif
