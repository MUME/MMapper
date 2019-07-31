#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <list>
#include <memory>
#include <QString>
#include <QtCore>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "path.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

class AbstractRoomFactory;
class Approved;
class Coordinate;
class Exit;
class MapAction;
class QEvent;
class QObject;
class RoomRecipient;
struct RoomId;

enum class PathStateEnum { APPROVED = 0, EXPERIMENTING = 1, SYNCING = 2 };

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
    PathParameters params;
    void experimenting(const SigParseEvent &sigParseEvent);
    void syncing(const SigParseEvent &sigParseEvent);
    void approved(const SigParseEvent &sigParseEvent);
    void evaluatePaths();
    void tryExits(const Room *, RoomRecipient &, ParseEvent &, bool out);
    void tryExit(const Exit &possible, RoomRecipient &recipient, bool out);
    void tryCoordinate(const Room *, RoomRecipient &, ParseEvent &);
    AbstractRoomFactory *factory = nullptr;

    RoomSignalHandler signaler;
    /* REVISIT: pathRoot and mostLikelyRoom should probably be of type RoomId */
    Room pathRoot;
    Room mostLikelyRoom;
    SigParseEvent lastEvent;
    PathStateEnum state = PathStateEnum::SYNCING;

    // TODO: use smart pointer to manage this.
    // It looks like it might be leaked because it's reassigned.
    PathList *paths = nullptr;

private:
    // avoid warning about signal hiding this function
    virtual bool event(QEvent *e) final override { return QObject::event(e); }
};
