#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/parseevent.h"
#include "../map/room.h"
#include "path.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

#include <list>
#include <memory>
#include <optional>

#include <QString>
#include <QtCore>

class Approved;
class Coordinate;
class Exit;
class MapAction;
class MapData;
class QEvent;
class QObject;
class RoomRecipient;
struct RoomId;

enum class NODISCARD PathStateEnum { APPROVED = 0, EXPERIMENTING = 1, SYNCING = 2 };

/**
 * the parser determines the relations between incoming move- and room-events
 * and decides if rooms have to be added (and where) and where the player is
 * the results are published via signals
 *
 * PathMachine is the base class for Mmapper2PathMachine
 */
class PathMachine : public QObject
{
    Q_OBJECT

protected:
    PathParameters params;
    MapData &m_mapData;
    RoomSignalHandler signaler;
    /* REVISIT: pathRoot and mostLikelyRoom should probably be of type RoomId */
    SigParseEvent lastEvent;
    PathStateEnum state = PathStateEnum::SYNCING;
    std::shared_ptr<PathList> paths;

private:
    std::optional<Coordinate> m_pathRootPos;
    std::optional<Coordinate> m_mostLikelyRoomPos;

public slots:
    void slot_releaseAllPaths();
    void slot_setCurrentRoom(RoomId id, bool update);
    void slot_scheduleAction(const std::shared_ptr<MapAction> &action) { scheduleAction(action); }

signals:
    void sig_lookingForRooms(RoomRecipient &, const SigParseEvent &);
    void sig_lookingForRooms(RoomRecipient &, RoomId);
    void sig_lookingForRooms(RoomRecipient &, const Coordinate &);
    void sig_playerMoved(const Coordinate &);
    void sig_createRoom(const SigParseEvent &, const Coordinate &);
    void sig_scheduleAction(std::shared_ptr<MapAction>);
    void sig_setCharPosition(RoomId id);

public:
    explicit PathMachine(MapData *mapData, QObject *parent);

protected:
    void handleParseEvent(const SigParseEvent &);

private:
    void scheduleAction(const std::shared_ptr<MapAction> &action);

protected:
    void experimenting(const SigParseEvent &sigParseEvent);
    void syncing(const SigParseEvent &sigParseEvent);
    void approved(const SigParseEvent &sigParseEvent);
    void evaluatePaths();
    void tryExits(const Room *, RoomRecipient &, const ParseEvent &, bool out);
    void tryExit(const Exit &possible, RoomRecipient &recipient, bool out);
    void tryCoordinate(const Room *, RoomRecipient &, const ParseEvent &);

private:
    void clearMostLikelyRoom() { m_mostLikelyRoomPos.reset(); }
    void setMostLikelyRoom(const Room &room) { m_mostLikelyRoomPos = room.getPosition(); }

protected:
    NODISCARD bool hasMostLikelyRoom() const { return m_mostLikelyRoomPos.has_value(); }
    NODISCARD const Room *getPathRoot() const;
    NODISCARD const Room *getMostLikelyRoom() const;
    NODISCARD RoomId getMostLikelyRoomId() const;
    NODISCARD const Coordinate &getMostLikelyRoomPosition() const;
};
