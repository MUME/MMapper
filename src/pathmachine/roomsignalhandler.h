#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <map>
#include <set>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

class MapAction;
class Room;
class RoomAdmin;
class RoomRecipient;
struct RoomId;

class RoomSignalHandler : public QObject
{
    Q_OBJECT
private:
    std::map<const Room *, RoomAdmin *> owners{};
    std::map<const Room *, std::set<RoomRecipient *>> lockers{};
    std::map<const Room *, int> holdCount{};

public:
    RoomSignalHandler() = delete;
    RoomSignalHandler(QObject *parent)
        : QObject(parent)
    {}
    /* receiving from our clients: */
    // hold the room, we don't know yet what to do, overrides release, re-caches if room was un-cached
    void hold(const Room *room, RoomAdmin *owner, RoomRecipient *locker);
    // room isn't needed anymore and can be deleted if no one else is holding it and no one else uncached it
    void release(const Room *room);
    // keep the room but un-cache it - overrides both hold and release
    // toId is negative if no exit should be added, else it's the id of
    // the room where the exit should lead
    void keep(const Room *room, ExitDirEnum dir, RoomId fromId);

    /* Sending to the rooms' owners:
       keepRoom: keep the room, but we don't need it anymore for now
       releaseRoom: delete the room, if you like */

    auto getNumLockers(const Room *room) { return lockers[room].size(); }

signals:
    void scheduleAction(MapAction *);
};
