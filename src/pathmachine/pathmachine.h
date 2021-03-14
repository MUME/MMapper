#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <list>
#include <memory>
#include <optional>
#include <QString>
#include <QtCore>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "path.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

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
public slots:
    // CAUTION: This hides virtual bool QObject::event(QEvent*).
    virtual void event(const SigParseEvent &);
    virtual void releaseAllPaths();
    virtual void setCurrentRoom(RoomId id, bool update);
    void slot_scheduleAction(const std::shared_ptr<MapAction> &action) { scheduleAction(action); }

signals:
    void lookingForRooms(RoomRecipient &, const SigParseEvent &);
    void lookingForRooms(RoomRecipient &, RoomId);
    void lookingForRooms(RoomRecipient &, const Coordinate &);
    void playerMoved(const Coordinate &);
    void createRoom(const SigParseEvent &, const Coordinate &);
    void sig_scheduleAction(std::shared_ptr<MapAction>);
    void setCharPosition(RoomId id);

public:
    explicit PathMachine(MapData *mapData, QObject *parent);

private:
    void scheduleAction(const std::shared_ptr<MapAction> &action);

protected:
    PathParameters params;
    MapData &m_mapData;

    void experimenting(const SigParseEvent &sigParseEvent);
    void syncing(const SigParseEvent &sigParseEvent);
    void approved(const SigParseEvent &sigParseEvent);
    void evaluatePaths();
    void tryExits(const Room *, RoomRecipient &, const ParseEvent &, bool out);
    void tryExit(const Exit &possible, RoomRecipient &recipient, bool out);
    void tryCoordinate(const Room *, RoomRecipient &, const ParseEvent &);

    RoomSignalHandler signaler;
    /* REVISIT: pathRoot and mostLikelyRoom should probably be of type RoomId */
    SigParseEvent lastEvent;
    PathStateEnum state = PathStateEnum::SYNCING;
    std::shared_ptr<PathList> paths;

private:
    std::optional<Coordinate> m_pathRootPos;
    std::optional<Coordinate> m_mostLikelyRoomPos;

private:
    void clearMostLikelyRoom() { m_mostLikelyRoomPos.reset(); }
    void setMostLikelyRoom(const Room &room) { m_mostLikelyRoomPos = room.getPosition(); }

protected:
    NODISCARD bool hasMostLikelyRoom() const { return m_mostLikelyRoomPos.has_value(); }
    NODISCARD const Room *getPathRoot() const;
    NODISCARD const Room *getMostLikelyRoom() const;
    NODISCARD RoomId getMostLikelyRoomId() const;
    NODISCARD const Coordinate &getMostLikelyRoomPosition() const;

private:
    // avoid warning about signal hiding this function
    virtual bool event(QEvent *e) final override { return QObject::event(e); }
};
