// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "pathmachine.h"

#include "../global/utils.h"
#include "../map/CommandId.h"
#include "../map/ConnectedRoomFlags.h"
#include "../map/ExitDirection.h"
#include "../map/coordinate.h"
#include "../map/exit.h"
#include "../map/mmapper2room.h"
#include "../map/room.h"
#include "../map/roomid.h"
#include "../mapdata/customaction.h"
#include "../mapdata/mapdata.h"
#include "../mapfrontend/mapaction.h"
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
#include <set>
#include <utility>

class RoomRecipient;

PathMachine::PathMachine(MapData *const mapData, QObject *const parent)
    : QObject(parent)
    , m_mapData{deref(mapData)}
    , m_signaler{this}
    , m_lastEvent{ParseEvent::createDummyEvent()}
    , m_paths{PathList::alloc()}
{
    connect(&m_signaler,
            &RoomSignalHandler::sig_scheduleAction,
            this,
            &PathMachine::slot_scheduleAction);
}

void PathMachine::slot_setCurrentRoom(const RoomId id, const bool update)
{
    Forced forced{m_lastEvent, update};
    emit sig_lookingForRooms(forced, id);
    slot_releaseAllPaths();
    if (const Room *const perhaps = forced.oneMatch()) {
        setMostLikelyRoom(*perhaps);
        emit sig_playerMoved(perhaps->getPosition());
        emit sig_setCharPosition(perhaps->getId());
        m_state = PathStateEnum::APPROVED;
    } else {
        clearMostLikelyRoom();
        m_state = PathStateEnum::SYNCING;
    }
}

void PathMachine::slot_releaseAllPaths()
{
    std::ignore = deref(m_paths);
    for (auto &path : *m_paths) {
        deref(path).deny();
    }
    m_paths->clear();

    m_state = PathStateEnum::SYNCING;
}

void PathMachine::handleParseEvent(const SigParseEvent &sigParseEvent)
{
    if (m_lastEvent != sigParseEvent.requireValid())
        m_lastEvent = sigParseEvent;

    m_lastEvent.requireValid();

    switch (m_state) {
    case PathStateEnum::APPROVED:
        approved(sigParseEvent);
        break;
    case PathStateEnum::EXPERIMENTING:
        experimenting(sigParseEvent);
        break;
    case PathStateEnum::SYNCING:
        syncing(sigParseEvent);
        break;
    }
}

void PathMachine::tryExits(const Room *const room,
                           RoomRecipient &recipient,
                           const ParseEvent &event,
                           const bool out)
{
    if (room == nullptr) {
        // most likely room doesn't exist
        return;
    }

    const CommandEnum move = event.getMoveType();
    if (isDirection7(move)) {
        const Exit &possible = room->exit(getDirection(move));
        tryExit(possible, recipient, out);
    } else {
        // Only check the current room for LOOK
        emit sig_lookingForRooms(recipient, room->getId());
        if (move >= CommandEnum::FLEE) {
            // Only try all possible exits for commands FLEE, SCOUT, and NONE
            for (const auto &possible : room->getExitsList()) {
                tryExit(possible, recipient, out);
            }
        }
    }
}

void PathMachine::tryExit(const Exit &possible, RoomRecipient &recipient, const bool out)
{
    for (auto idx : possible.getRange(out)) {
        emit sig_lookingForRooms(recipient, idx);
    }
}

void PathMachine::tryCoordinate(const Room *const room,
                                RoomRecipient &recipient,
                                const ParseEvent &event)
{
    if (room == nullptr) {
        // most likely room doesn't exist
        return;
    }

    const CommandEnum moveCode = event.getMoveType();
    if (moveCode < CommandEnum::FLEE) {
        // LOOK, UNKNOWN will have an empty offset
        auto offset = Room::exitDir(getDirection(moveCode));
        const Coordinate c = room->getPosition() + offset;
        emit sig_lookingForRooms(recipient, c);

    } else {
        const Coordinate roomPos = room->getPosition();
        // REVISIT: Should this enumerate 6 or 7 values?
        // NOTE: This previously enumerated 8 values instead of 7,
        // which meant it was asking for exitDir(ExitDirEnum::NONE),
        // even though both ExitDirEnum::UNKNOWN and ExitDirEnum::NONE
        // both have Coordinate(0, 0, 0).
        for (const ExitDirEnum dir : ALL_EXITS7) {
            emit sig_lookingForRooms(recipient, roomPos + Room::exitDir(dir));
        }
    }
}

void PathMachine::approved(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();

    Approved appr{sigParseEvent, m_params.matchingTolerance};
    const Room *perhaps = nullptr;

    if (event.getMoveType() == CommandEnum::LOOK) {
        emit sig_lookingForRooms(appr, getMostLikelyRoomId());

    } else {
        tryExits(getMostLikelyRoom(), appr, event, true);
    }

    perhaps = appr.oneMatch();

    if (perhaps == nullptr) {
        // try to match by reverse exit
        appr.releaseMatch();
        tryExits(getMostLikelyRoom(), appr, event, false);
        perhaps = appr.oneMatch();
        if (perhaps == nullptr) {
            // try to match by coordinate
            appr.releaseMatch();
            tryCoordinate(getMostLikelyRoom(), appr, event);
            perhaps = appr.oneMatch();
            if (perhaps == nullptr) {
                // try to match by coordinate one step below expected
                // FIXME: need stronger type checking here.

                const auto cmd = event.getMoveType();
                // NOTE: This allows ExitDirEnum::UNKNOWN,
                // which means the coordinate can be Coordinate(0,0,0).
                const Coordinate &eDir = Room::exitDir(getDirection(cmd));

                // CAUTION: This test seems to mean it wants only NESW,
                // but it would also accept ExitDirEnum::UNKNOWN,
                // which in the context of this function would mean "no move."
                if (eDir.z == 0) {
                    appr.releaseMatch();
                    Coordinate c = getMostLikelyRoomPosition() + eDir;
                    c.z--;
                    emit sig_lookingForRooms(appr, c);
                    perhaps = appr.oneMatch();

                    if (perhaps == nullptr) {
                        // try to match by coordinate one step above expected
                        appr.releaseMatch();
                        c.z += 2;
                        emit sig_lookingForRooms(appr, c);
                        perhaps = appr.oneMatch();
                    }
                }
            }
        }
    }

    if (perhaps == nullptr) {
        // couldn't match, give up
        m_state = PathStateEnum::EXPERIMENTING;
        m_pathRootPos = m_mostLikelyRoomPos;

        const Room *const pathRoot = getPathRoot();
        if (pathRoot == nullptr) {
            // What do we do now? "Who cares?"
            return;
        }

        deref(m_paths).push_front(
            Path::alloc(pathRoot, nullptr, nullptr, &m_signaler, std::nullopt));
        experimenting(sigParseEvent);

        return;
    }

    // Update the exit from the previous room to the current room
    const CommandEnum move = event.getMoveType();
    if (isDirection7(move)) {
        if (const Room *const pRoom = getMostLikelyRoom()) {
            const auto dir = getDirection(move);
            const auto &mostLikelyExit = pRoom->exit(dir);
            if (!mostLikelyExit.containsOut(perhaps->getId())) {
                scheduleAction(
                    std::make_shared<AddExit>(getMostLikelyRoomId(), perhaps->getId(), dir));
            }
        }
    }

    // Update most likely room with player's current location
    setMostLikelyRoom(*perhaps);

    // Update rooms behind exits now that we are certain about our current location
    const ConnectedRoomFlagsType bFlags = event.getConnectedRoomFlags();
    if (bFlags.isValid() && (bFlags.hasAnyDirectSunlight() || bFlags.isTrollMode())) {
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
            // guaranteed to succeed, since it's set above.
            const Exit &e = getMostLikelyRoom()->exit(dir);
            if (!e.outIsUnique()) {
                continue;
            }
            const RoomId connectedRoomId = e.outFirst();
            if (bFlags.hasDirectSunlight(dir)) {
                // Allow orcs to set Sundeath
                scheduleAction(std::make_shared<SingleRoomAction>(
                    std::make_unique<ModifyRoomFlags>(RoomSundeathEnum::SUNDEATH,
                                                      FlagModifyModeEnum::SET),
                    connectedRoomId));
            } else if (bFlags.isTrollMode() && bFlags.hasNoDirectSunlight(dir)) {
                // Only trust troll mode to set No-Sundeath
                scheduleAction(std::make_shared<SingleRoomAction>(
                    std::make_unique<ModifyRoomFlags>(RoomSundeathEnum::NO_SUNDEATH,
                                                      FlagModifyModeEnum::SET),
                    connectedRoomId));
            }
        }
    }

    // Update the room if we had a tolerant match rather than an exact match
    if (appr.needsUpdate()) {
        scheduleAction(std::make_shared<SingleRoomAction>(std::make_unique<Update>(sigParseEvent),
                                                          getMostLikelyRoomId()));
    }

    // Send updates
    emit sig_playerMoved(getMostLikelyRoomPosition());
    // GroupManager
    emit sig_setCharPosition(getMostLikelyRoomId());
}

void PathMachine::syncing(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();
    {
        Syncing sync{m_params, m_paths, &m_signaler};
        if (event.getNumSkipped() <= m_params.maxSkipped) {
            emit sig_lookingForRooms(sync, sigParseEvent);
        }
        m_paths = sync.evaluate();
    }
    evaluatePaths();
}

void PathMachine::experimenting(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();

    std::unique_ptr<Experimenting> exp;
    const CommandEnum moveCode = event.getMoveType();

    const auto dir = getDirection(moveCode);
    const Coordinate &move = Room::exitDir(dir);

    // only create rooms if no properties are skipped and
    // the move coordinate is not 0,0,0

    if (event.getNumSkipped() == 0 && moveCode < CommandEnum::FLEE && hasMostLikelyRoom()
        && !move.isNull()) {
        exp = std::make_unique<Crossover>(m_paths, dir, m_params);
        std::set<const Room *> pathEnds{};
        for (auto &path : deref(m_paths)) {
            const Room *const working = deref(path).getRoom();
            if (pathEnds.find(working) == pathEnds.end()) {
                emit sig_createRoom(sigParseEvent, working->getPosition() + move);
                pathEnds.insert(working);
            }
        }
        emit sig_lookingForRooms(*exp, sigParseEvent);
    } else {
        auto pOneByOne = std::make_unique<OneByOne>(sigParseEvent, m_params, &m_signaler);
        {
            auto &tmp = *pOneByOne;
            for (auto &path : deref(m_paths)) {
                const Room *const working = path->getRoom();
                tmp.addPath(path);
                tryExits(working, tmp, event, true);
                tryExits(working, tmp, event, false);
                tryCoordinate(working, tmp, event);
            }
        }
        exp = static_upcast<Experimenting>(std::exchange(pOneByOne, nullptr));
    }

    m_paths = exp->evaluate();
    evaluatePaths();
}

void PathMachine::evaluatePaths()
{
    auto &paths = deref(m_paths);
    if (paths.empty()) {
        m_state = PathStateEnum::SYNCING;
    } else {
        if (const Room *const room = (paths.front()->getRoom()))
            setMostLikelyRoom(*room);
        else
            clearMostLikelyRoom();

        if (std::next(paths.begin()) == paths.end()) {
            m_state = PathStateEnum::APPROVED;
            std::shared_ptr<Path> path = utils::pop_front(deref(m_paths));
            deref(path).approve();
        } else {
            m_state = PathStateEnum::EXPERIMENTING;
        }
        emit sig_playerMoved(getMostLikelyRoomPosition());
        emit sig_setCharPosition(getMostLikelyRoomId());
    }
}

void PathMachine::scheduleAction(const std::shared_ptr<MapAction> &action)
{
    emit sig_scheduleAction(action);
}

const Room *PathMachine::getPathRoot() const
{
    return m_mapData.getRoom(m_pathRootPos.value_or(Coordinate{}));
}

const Room *PathMachine::getMostLikelyRoom() const
{
    return m_mapData.getRoom(m_mostLikelyRoomPos.value_or(Coordinate{}));
}

RoomId PathMachine::getMostLikelyRoomId() const
{
    if (const Room *const room = getMostLikelyRoom())
        return room->getId();

    return INVALID_ROOMID;
}

const Coordinate &PathMachine::getMostLikelyRoomPosition() const
{
    if (const Room *const room = getMostLikelyRoom())
        return room->getPosition();

    static const Coordinate fake;
    return fake;
}
