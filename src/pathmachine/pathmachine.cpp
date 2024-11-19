// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "pathmachine.h"

#include "../global/logging.h"
#include "../global/utils.h"
#include "../map/ChangeList.h"
#include "../map/ChangeTypes.h"
#include "../map/CommandId.h"
#include "../map/ConnectedRoomFlags.h"
#include "../map/ExitDirection.h"
#include "../map/coordinate.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/mapdata.h"
#include "approved.h"
#include "crossover.h"
#include "experimenting.h"
#include "forced.h"
#include "onebyone.h"
#include "path.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"
#include "syncing.h"

#include <cassert>
#include <memory>
#include <optional>
#include <set>
#include <unordered_set>
#include <utility>

class RoomRecipient;

PathMachine::PathMachine(MapFrontend &map, QObject *const parent)
    : QObject(parent)
    , m_map{map}
    , m_signaler{map, this}
    , m_lastEvent{ParseEvent::createDummyEvent()}
    , m_paths{PathList::alloc()}
{
    connect(&m_signaler,
            &RoomSignalHandler::sig_scheduleAction,
            this,
            &PathMachine::slot_scheduleAction);
}

bool PathMachine::hasLastEvent() const
{
    if (!m_lastEvent.isValid()) {
        return false;
    }

    ParseEvent &event = m_lastEvent.deref();
    return event.canCreateNewRoom();
}

void PathMachine::forcePositionChange(const RoomId id, const bool update)
{
    slot_releaseAllPaths();

    if (id == INVALID_ROOMID) {
        MMLOG() << "in " << __FUNCTION__ << " detected invalid room.";

        clearMostLikelyRoom();
        m_state = PathStateEnum::SYNCING;
        return;
    }

    const auto &room = m_map.findRoomHandle(id);
    if (!room) {
        clearMostLikelyRoom();
        m_state = PathStateEnum::SYNCING;
        return;
    }

    setMostLikelyRoom(id);
    emit sig_playerMoved(id);
    publishExternalId();
    m_state = PathStateEnum::APPROVED;

    if (!update) {
        return;
    }

    if (!hasLastEvent()) {
        MMLOG() << "in " << __FUNCTION__ << " has invalid event.";
        return;
    }

    // Force update room with last event
    m_map.scheduleAction(Change{room_change_types::Update{id, m_lastEvent.deref()}});
}

void PathMachine::slot_releaseAllPaths()
{
    auto &paths = deref(m_paths);
    for (auto &path : paths) {
        path->deny();
    }
    paths.clear();

    m_state = PathStateEnum::SYNCING;

    // REVISIT: should these be cleared, too?
    if ((false)) {
        m_pathRoot.reset();
        m_mostLikelyRoom.reset();
    }
}

void PathMachine::handleParseEvent(const SigParseEvent &sigParseEvent)
{
    if (m_lastEvent != sigParseEvent.requireValid()) {
        m_lastEvent = sigParseEvent;
    }

    m_lastEvent.requireValid();

    ChangeList changes;

    switch (m_state) {
    case PathStateEnum::APPROVED:
        changes = approved(sigParseEvent);
        break;
    case PathStateEnum::EXPERIMENTING:
        experimenting(sigParseEvent);
        break;
    case PathStateEnum::SYNCING:
        syncing(sigParseEvent);
        break;
    }

    if (m_state == PathStateEnum::APPROVED && hasMostLikelyRoom()) {
        updateMostLikelyRoom(sigParseEvent, changes);
    }
}

void PathMachine::tryExits(const RoomPtr &room,
                           RoomRecipient &recipient,
                           const ParseEvent &event,
                           const bool out)
{
    if (room == std::nullopt) {
        // most likely room doesn't exist
        return;
    }

    const CommandEnum move = event.getMoveType();
    if (isDirection7(move)) {
        const auto &possible = room->getExit(getDirection(move));
        tryExit(possible, recipient, out);
    } else {
        // Only check the current room for LOOK
        m_map.lookingForRooms(recipient, room->getId());
        if (move >= CommandEnum::FLEE) {
            // Only try all possible exits for commands FLEE, SCOUT, and NONE
            for (const auto &possible : room->getExits()) {
                tryExit(possible, recipient, out);
            }
        }
    }
}

void PathMachine::tryExit(const RawExit &possible, RoomRecipient &recipient, const bool out)
{
    for (auto idx : (out ? possible.getOutgoingSet() : possible.getIncomingSet())) {
        m_map.lookingForRooms(recipient, idx);
    }
}

void PathMachine::tryCoordinate(const RoomPtr &room,
                                RoomRecipient &recipient,
                                const ParseEvent &event)
{
    if (room == std::nullopt) {
        // most likely room doesn't exist
        return;
    }

    const CommandEnum moveCode = event.getMoveType();
    if (moveCode < CommandEnum::FLEE) {
        // LOOK, UNKNOWN will have an empty offset
        auto offset = ::exitDir(getDirection(moveCode));
        const Coordinate c = room->getPosition() + offset;
        m_map.lookingForRooms(recipient, c);

    } else {
        const Coordinate roomPos = room->getPosition();
        // REVISIT: Should this enumerate 6 or 7 values?
        // NOTE: This previously enumerated 8 values instead of 7,
        // which meant it was asking for exitDir(ExitDirEnum::NONE),
        // even though both ExitDirEnum::UNKNOWN and ExitDirEnum::NONE
        // both have Coordinate(0, 0, 0).
        for (const ExitDirEnum dir : ALL_EXITS7) {
            m_map.lookingForRooms(recipient, roomPos + ::exitDir(dir));
        }
    }
}

ChangeList PathMachine::approved(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();

    RoomPtr perhaps;
    Approved appr{m_map, sigParseEvent, m_params.matchingTolerance};

    if (event.getServerId() != INVALID_SERVER_ROOMID) {
        perhaps = m_map.findRoomHandle(event.getServerId());
        if (perhaps.has_value()) {
            appr.receiveRoom(perhaps.value());
        }
        perhaps = appr.oneMatch();
    }

    // This code path only happens for historic maps and mazes where no server id is present
    if (perhaps == std::nullopt) {
        appr.releaseMatch();

        tryExits(getMostLikelyRoom(), appr, event, true);
        perhaps = appr.oneMatch();

        if (perhaps == std::nullopt) {
            // try to match by reverse exit
            appr.releaseMatch();
            tryExits(getMostLikelyRoom(), appr, event, false);
            perhaps = appr.oneMatch();
            if (perhaps == std::nullopt) {
                // try to match by coordinate
                appr.releaseMatch();
                tryCoordinate(getMostLikelyRoom(), appr, event);
                perhaps = appr.oneMatch();
                if (perhaps == std::nullopt) {
                    // try to match by coordinate one step below expected
                    appr.releaseMatch();
                    // FIXME: need stronger type checking here.

                    const auto cmd = event.getMoveType();
                    // NOTE: This allows ExitDirEnum::UNKNOWN,
                    // which means the coordinate can be Coordinate(0,0,0).
                    const Coordinate &eDir = ::exitDir(getDirection(cmd));

                    // CAUTION: This test seems to mean it wants only NESW,
                    // but it would also accept ExitDirEnum::UNKNOWN,
                    // which in the context of this function would mean "no move."
                    if (eDir.z == 0) {
                        // REVISIT: if we can be sure that this function does not modify the
                        // mostLikelyRoom, then we should retrieve it above, and directly ask
                        // for its position here instead of using this fragile interface.
                        if (const auto &pos = tryGetMostLikelyRoomPosition()) {
                            Coordinate c = pos.value() + eDir;
                            c.z--;
                            m_map.lookingForRooms(appr, c);
                            perhaps = appr.oneMatch();

                            if (perhaps == std::nullopt) {
                                // try to match by coordinate one step above expected
                                appr.releaseMatch();
                                c.z += 2;
                                m_map.lookingForRooms(appr, c);
                                perhaps = appr.oneMatch();
                            }
                        }
                    }
                }
            }
        }
    }

    if (perhaps == std::nullopt) {
        // couldn't match, give up
        m_state = PathStateEnum::EXPERIMENTING;
        m_pathRoot = m_mostLikelyRoom;

        const RoomPtr &pathRoot = getPathRoot();
        if (pathRoot == std::nullopt) {
            // What do we do now? "Who cares?"
            return ChangeList{};
        }

        /* FIXME: null locker ends up being an error in RoomSignalHandler::keep(),
         * so why is this allowed to be null, and how do we prevent this null
         * from actually causing an error? */
        deref(m_paths).push_front(Path::alloc(pathRoot, nullptr, &m_signaler, std::nullopt));
        experimenting(sigParseEvent);

        return ChangeList{};
    }

    ChangeList changes;

    // Update the exit from the previous room to the current room
    const CommandEnum move = event.getMoveType();
    if (isDirectionNESWUD(move)) {
        if (const RoomPtr &pRoom = getMostLikelyRoom()) {
            const auto &room = deref(pRoom);
            const auto dir = getDirection(move);
            const auto &ex = room.getExit(dir);
            const auto to = perhaps->getId();
            if (!ex.containsOut(to)) {
                const auto from = room.getId();
                changes.add(Change{exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                                           from,
                                                                           dir,
                                                                           to,
                                                                           WaysEnum::OneWay}});
            }
        }
    }

    // Update most likely room with player's current location
    setMostLikelyRoom(perhaps->getId());

    updateMostLikelyRoom(sigParseEvent, changes);

    // Update the room if we had a tolerant match rather than an exact match
    if (appr.needsUpdate()) {
        changes.add(Change{room_change_types::Update{getMostLikelyRoomId(), sigParseEvent.deref()}});
    }

    emit sig_playerMoved(getMostLikelyRoomId());
    publishExternalId();
    return changes;
}

void PathMachine::updateMostLikelyRoom(const SigParseEvent &sigParseEvent, ChangeList &changes)
{
    ParseEvent &event = sigParseEvent.deref();

    // guaranteed to succeed, since it's set above.
    const auto pHere = getMostLikelyRoom();
    const auto &here = deref(pHere);

    // track added ServerIds to prevent multiple allocations
    std::unordered_set<ServerRoomId> addedIds;

    if (event.getServerId() != INVALID_SERVER_ROOMID) {
        const auto oldId = here.getServerId();
        const auto newId = event.getServerId();
        if (oldId == INVALID_SERVER_ROOMID && newId != INVALID_SERVER_ROOMID) {
            changes.add(Change{room_change_types::SetServerId{here.getId(), newId}});
            addedIds.emplace(newId);
            qInfo() << "Set server id" << newId.asUint32();
        }
    }

    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const auto toServerId = event.getExitIds()[dir];
        if (toServerId == INVALID_SERVER_ROOMID) {
            continue;
        }
        const auto &e = here.getExit(dir);
        if (const auto &pThere = m_map.findRoomHandle(toServerId)) {
            const auto &there = deref(pThere);
            // ServerId already exists
            const auto from = here.getId();
            const auto to = there.getId();
            if (!e.containsOut(to)) {
                changes.add(Change{exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                                           from,
                                                                           dir,
                                                                           to,
                                                                           WaysEnum::OneWay}});
            }
        } else if (e.outIsUnique() && addedIds.find(toServerId) == addedIds.end()) {
            // Add likely ServerId
            const auto to = e.outFirst();
            changes.add(Change{room_change_types::SetServerId{to, toServerId}});
            addedIds.emplace(toServerId);
        }
    }

    // Update rooms behind exits now that we are certain about our current location
    if (const ConnectedRoomFlagsType crf = event.getConnectedRoomFlags();
        crf.isValid() && (crf.hasAnyDirectSunlight() || crf.isTrollMode())) {
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
            const auto &e = here.getExit(dir);
            if (!e.outIsUnique()) {
                continue;
            }

            const RoomId to = e.outFirst();
            if (crf.hasDirectSunlight(dir)) {
                changes.add(Change{room_change_types::ModifyRoomFlags{to,
                                                                      RoomSundeathEnum::SUNDEATH,
                                                                      FlagModifyModeEnum::ASSIGN}});
            } else if (crf.isTrollMode() && crf.hasNoDirectSunlight(dir)) {
                changes.add(Change{room_change_types::ModifyRoomFlags{to,
                                                                      RoomSundeathEnum::NO_SUNDEATH,
                                                                      FlagModifyModeEnum::ASSIGN}});
            }
        }
    }

    if (!changes.empty()) {
        scheduleAction(changes);
    }
}

void PathMachine::syncing(const SigParseEvent &sigParseEvent)
{
    auto &params = m_params;
    ParseEvent &event = sigParseEvent.deref();
    {
        Syncing sync{params, m_paths, &m_signaler};
        if (event.getServerId() != INVALID_SERVER_ROOMID
            || event.getNumSkipped() <= params.maxSkipped) {
            m_map.lookingForRooms(sync, sigParseEvent);
        }
        m_paths = sync.evaluate();
    }
    evaluatePaths();
}

void PathMachine::experimenting(const SigParseEvent &sigParseEvent)
{
    auto &params = m_params;
    ParseEvent &event = sigParseEvent.deref();
    const CommandEnum moveCode = event.getMoveType();

    // only create rooms if it has a serverId or no properties are skipped and the direction is NESWUD.
    if (event.canCreateNewRoom() && isDirectionNESWUD(moveCode) && hasMostLikelyRoom()) {
        const auto dir = getDirection(moveCode);
        const Coordinate &move = ::exitDir(dir);
        Crossover exp{m_map, m_paths, dir, params};
        RoomIdSet pathEnds;
        for (const auto &path : deref(m_paths)) {
            const auto &working = path->getRoom();
            const RoomId workingId = deref(working).getId();
            if (!pathEnds.contains(workingId)) {
                qInfo() << "creating RoomId" << workingId.asUint32();
                emit sig_createRoom(sigParseEvent, working->getPosition() + move);
                pathEnds.insert(workingId);
            }
        }
        // Look for appropriate rooms (including those we just created)
        m_map.lookingForRooms(exp, sigParseEvent);
        m_paths = exp.evaluate();
    } else {
        OneByOne oneByOne{sigParseEvent, params, &m_signaler};
        {
            auto &tmp = oneByOne;
            for (const auto &path : deref(m_paths)) {
                const auto &working = path->getRoom();
                tmp.addPath(path);
                tryExits(working, tmp, event, true);
                tryExits(working, tmp, event, false);
                tryCoordinate(working, tmp, event);
            }
        }
        m_paths = oneByOne.evaluate();
    }

    evaluatePaths();
}

void PathMachine::evaluatePaths()
{
    auto &paths = deref(m_paths);
    if (paths.empty()) {
        m_state = PathStateEnum::SYNCING;
        return;
    }

    if (const RoomPtr &room = paths.front()->getRoom()) {
        setMostLikelyRoom(room->getId());
    } else {
        // REVISIT: Should this case set state to SYNCING and then return?
        clearMostLikelyRoom();
    }

    // FIXME: above establishes that the room might not exist,
    // but here we happily assume that it does exist.
    if (paths.size() == 1) {
        m_state = PathStateEnum::APPROVED;
        std::shared_ptr<Path> path = utils::pop_front(paths);
        deref(path).approve();
    } else {
        m_state = PathStateEnum::EXPERIMENTING;
    }

    emit sig_playerMoved(getMostLikelyRoomId());
    publishExternalId();
}

void PathMachine::scheduleAction(const ChangeList &action)
{
    if ((false)) {
        // note: sig_scheduleAction conditionally calls in MAP mode so we'd need something
        // like `if (m_mode == MapModeEnum::MAP)` somewhere if we're going to reproduce the
        // same behavior without signals:
        m_map.applyChanges(action);
    } else {
        const auto wrapped = SigMapChangeList{std::make_shared<ChangeList>(action)};
        emit sig_scheduleAction(wrapped);
    }
}

RoomPtr PathMachine::getPathRoot() const
{
    if (!m_pathRoot.has_value()) {
        return std::nullopt;
    }

    return m_map.findRoomHandle(m_pathRoot.value());
}

RoomPtr PathMachine::getMostLikelyRoom() const
{
    if (!m_mostLikelyRoom.has_value()) {
        return std::nullopt;
    }

    return m_map.findRoomHandle(m_mostLikelyRoom.value());
}

RoomId PathMachine::getMostLikelyRoomId() const
{
    if (const RoomPtr &room = getMostLikelyRoom()) {
        return room->getId();
    }

    return INVALID_ROOMID;
}

Coordinate PathMachine::getMostLikelyRoomPosition() const
{
    return tryGetMostLikelyRoomPosition().value_or(Coordinate{});
}

void PathMachine::setMostLikelyRoom(const RoomId roomId)
{
    if (const auto &room = m_map.findRoomHandle(roomId)) {
        m_mostLikelyRoom = roomId;
    } else {
        m_mostLikelyRoom.reset();
    }
}

void PathMachine::publishExternalId()
{
    const auto roomId = getMostLikelyRoomId();
    if (roomId == INVALID_ROOMID) {
        return;
    }

    auto room = m_map.findRoomHandle(roomId);
    if (!room) {
        return;
    }

    setCharRoomIdEstimated(room->getServerId(), room->getIdExternal());
}

std::optional<Coordinate> PathMachine::tryGetMostLikelyRoomPosition() const
{
    if (auto room = getMostLikelyRoom()) {
        return room->getPosition();
    }
    return std::nullopt;
}
