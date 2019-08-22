#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <map>
#include <set>
#include <stack>
#include <QMutex>
#include <QString>
#include <QtCore>

#include "../expandoracommon/RoomAdmin.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "../global/roomid.h"
#include "ParseTree.h"
#include "map.h"

class AbstractRoomFactory;
class Frustum;
class MapAction;
class ParseEvent;
class QObject;
class Room;
class RoomCollection;
class RoomRecipient;

/**
 * The MapFrontend organizes rooms and their relations to each other.
 */
class MapFrontend : public QObject, public RoomAdmin
{
    Q_OBJECT
    friend class FrontendAccessor;

protected:
    ParseTree parseTree{};
    Map map{};
    RoomIndex roomIndex{};
    std::stack<RoomId> unusedIds{};
    std::map<RoomId, std::set<MapAction *>> actionSchedule{};
    RoomHomes roomHomes{};
    RoomLocks locks{};

    RoomId greatestUsedId = INVALID_ROOMID;
    QMutex mapLock{};
    Coordinate ulf{};
    Coordinate lrb{};
    AbstractRoomFactory *factory = nullptr;

    void executeActions(RoomId roomId);
    void executeAction(MapAction *action);
    bool isExecutable(MapAction *action);
    void removeAction(MapAction *action);

    virtual RoomId assignId(Room *room, const SharedRoomCollection &roomHome);
    virtual void checkSize(const Coordinate &);

public:
    explicit MapFrontend(AbstractRoomFactory *factory, QObject *parent = nullptr);
    virtual ~MapFrontend() override;
    virtual void clear();
    void block();
    void unblock();
    virtual void checkSize();

    // removes the lock on a room
    // after the last lock is removed, the room is deleted
    virtual void releaseRoom(RoomRecipient &, RoomId) override;

    // makes a lock on a room permanent and anonymous.
    // Like that the room can't be deleted via releaseRoom anymore.
    virtual void keepRoom(RoomRecipient &, RoomId) override;

    virtual void lockRoom(RoomRecipient *, RoomId);

    virtual RoomId createEmptyRoom(const Coordinate &);

    virtual void insertPredefinedRoom(Room &);

    virtual RoomId getMaxId() { return greatestUsedId; }

    virtual const Coordinate &getUlf() const { return ulf; }
    virtual const Coordinate &getLrb() const { return lrb; }
public slots:
    // looking for rooms leads to a bunch of foundRoom() signals
    virtual void lookingForRooms(RoomRecipient &, const SigParseEvent &);
    virtual void lookingForRooms(RoomRecipient &, RoomId); // by id
    virtual void lookingForRooms(RoomRecipient &, const Coordinate &);
    virtual void lookingForRooms(RoomRecipient &, const Frustum &);
    virtual void lookingForRooms(RoomRecipient &,
                                 const Coordinate &,
                                 const Coordinate &); // by bounding box

    // createRoom creates a room without a lock
    // it will get deleted if no one looks for it for a certain time
    virtual void createRoom(const SigParseEvent &, const Coordinate &);
    virtual void createEmptyRooms(const Coordinate &, const Coordinate &);

    virtual void scheduleAction(MapAction *action) override;

signals:

    // this signal is sent out if a room is deleted. So any clients still
    // working on this room can start some emergency action.
    void mapSizeChanged(const Coordinate &, const Coordinate &);
    void clearingMap();
};
