#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../expandoracommon/RoomAdmin.h"
#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../map/parseevent.h"
#include "../map/roomid.h"
#include "../map/utils.h"
#include "ParseTree.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>

#include <QMutex>
#include <QString>
#include <QtCore>

class MapAction;
class ParseEvent;
class QObject;
class Room;
class RoomCollection;
class RoomRecipient;

/**
 * The MapFrontend organizes rooms and their relations to each other.
 */
class NODISCARD_QOBJECT MapFrontend : public QObject,
                                      public RoomAdmin,
                                      public InfoMarkModificationTracker
{
    Q_OBJECT

private:
    friend class FrontendAccessor;

protected:
    ParseTree parseTree;
    Map map;
    RoomIndex roomIndex;
    std::stack<RoomId> unusedIds;
    std::map<RoomId, std::set<std::shared_ptr<MapAction>>> actionSchedule;
    RoomHomes roomHomes;
    RoomLocks locks;

    RoomId greatestUsedId = INVALID_ROOMID;
    QRecursiveMutex mapLock;

    struct Bounds final
    {
        Coordinate min;
        Coordinate max;
    };
    std::optional<Bounds> m_bounds;

    void executeActions(RoomId roomId);
    void executeAction(MapAction *action);
    bool isExecutable(MapAction *action);
    void removeAction(const std::shared_ptr<MapAction> &action);

    RoomId assignId(const SharedRoom &room, const SharedRoomCollection &roomHome);
    void checkSize(const Coordinate &);

public:
    explicit MapFrontend(QObject *parent);
    ~MapFrontend() override;

private:
    virtual void virt_clear() = 0;

public:
    void clear();
    void block();
    void unblock();
    void checkSize();

    // removes the lock on a room
    // after the last lock is removed, the room is deleted
    void releaseRoom(RoomRecipient &, RoomId) final;

    // makes a lock on a room permanent and anonymous.
    // Like that the room can't be deleted via releaseRoom anymore.
    void keepRoom(RoomRecipient &, RoomId) final;

    void lockRoom(RoomRecipient *, RoomId);
    RoomId createEmptyRoom(const Coordinate &);
    void insertPredefinedRoom(const SharedRoom &);
    RoomId getMaxId() { return greatestUsedId; }
    Coordinate getMin() const { return m_bounds ? m_bounds->min : Coordinate{}; }
    Coordinate getMax() const { return m_bounds ? m_bounds->max : Coordinate{}; }

public:
    void scheduleAction(const std::shared_ptr<MapAction> &action) final;

signals:
    // this signal is also sent out if a room is deleted. So any clients still
    // working on this room can start some emergency action.
    void sig_mapSizeChanged(const Coordinate &, const Coordinate &);
    void sig_clearingMap();

public slots:
    // looking for rooms leads to a bunch of foundRoom() signals
    void lookingForRooms(RoomRecipient &, const SigParseEvent &);
    void lookingForRooms(RoomRecipient &, RoomId); // by id
    void lookingForRooms(RoomRecipient &, const Coordinate &);
    void lookingForRooms(RoomRecipient &,
                         const Coordinate &,
                         const Coordinate &); // by bounding box

    // createRoom creates a room without a lock
    // it will get deleted if no one looks for it for a certain time
    void slot_createRoom(const SigParseEvent &, const Coordinate &);

    void slot_scheduleAction(std::shared_ptr<MapAction> action) { scheduleAction(action); }
};
