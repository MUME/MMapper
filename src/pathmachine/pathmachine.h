#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../configuration/configuration.h"
#include "../map/ChangeList.h"
#include "../map/parseevent.h"
#include "../map/room.h"
#include "path.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"

#include <memory>
#include <optional>

#include <QString>
#include <QtCore>

class Approved;
class Coordinate;
class MapFrontend;
class QEvent;
class QObject;
class RoomRecipient;
struct RoomId;

enum class NODISCARD PathStateEnum : uint8_t { APPROVED = 0, EXPERIMENTING = 1, SYNCING = 2 };

/**
 * the parser determines the relations between incoming move- and room-events
 * and decides if rooms have to be added (and where) and where the player is
 * the results are published via signals
 *
 * PathMachine is the base class for Mmapper2PathMachine
 */
class NODISCARD_QOBJECT PathMachine : public QObject
{
    Q_OBJECT

protected:
    PathParameters m_params;

private:
    MapFrontend &m_map;
    RoomSignalHandler m_signaler;
    SigParseEvent m_lastEvent;
    std::shared_ptr<PathList> m_paths;
    std::optional<RoomId> m_pathRoot;
    std::optional<RoomId> m_mostLikelyRoom;
    PathStateEnum m_state = PathStateEnum::SYNCING;

public:
    void onPositionChange(std::optional<RoomId> optId)
    {
        forcePositionChange(optId.value_or(INVALID_ROOMID), false);
    }
    void forceUpdate(const RoomId id) { forcePositionChange(id, true); }
    NODISCARD bool hasLastEvent() const;

public:
    void onMapLoaded();

protected:
    explicit PathMachine(MapFrontend &map, QObject *parent);

protected:
    void handleParseEvent(const SigParseEvent &);

private:
    void scheduleAction(const ChangeList &action);
    void forcePositionChange(RoomId id, bool update);

private:
    void experimenting(const SigParseEvent &sigParseEvent, ChangeList &changes);
    void syncing(const SigParseEvent &sigParseEvent, ChangeList &changes);
    void approved(const SigParseEvent &sigParseEvent, ChangeList &changes);
    void evaluatePaths(ChangeList &changes);
    void tryExits(const RoomHandle &, RoomRecipient &, const ParseEvent &, bool out);
    void tryExit(const RawExit &possible, RoomRecipient &recipient, bool out);
    void tryCoordinate(const RoomHandle &, RoomRecipient &, const ParseEvent &);

private:
    void updateMostLikelyRoom(const SigParseEvent &sigParseEvent, ChangeList &changes, bool force);

private:
    void clearMostLikelyRoom() { m_mostLikelyRoom.reset(); }
    void setMostLikelyRoom(RoomId roomId);

protected:
    NODISCARD PathStateEnum getState() const { return m_state; }
    NODISCARD MapModeEnum getMapMode() const { return getConfig().general.mapMode; }

private:
    NODISCARD bool hasMostLikelyRoom() const { return m_mostLikelyRoom.has_value(); }
    // NOTE: This can fail.
    NODISCARD std::optional<Coordinate> tryGetMostLikelyRoomPosition() const;
    // NOTE: This can fail.
    NODISCARD RoomHandle getPathRoot() const;
    // NOTE: This can fail.
    NODISCARD RoomHandle getMostLikelyRoom() const;

signals:
    void sig_playerMoved(RoomId id);

public slots:
    void slot_releaseAllPaths();
};
