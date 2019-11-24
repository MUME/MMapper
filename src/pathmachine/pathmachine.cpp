// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "pathmachine.h"

#include <cassert>
#include <memory>
#include <set>
#include <utility>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/customaction.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/mmapper2room.h"
#include "../mapfrontend/mapaction.h"
#include "../parser/CommandId.h"
#include "../parser/ConnectedRoomFlags.h"
#include "approved.h"
#include "crossover.h"
#include "experimenting.h"
#include "forced.h"
#include "onebyone.h"
#include "path.h"
#include "pathparameters.h"
#include "roomsignalhandler.h"
#include "syncing.h"

class RoomRecipient;

PathMachine::PathMachine(MapData *const mapData, QObject *const parent)
    : QObject(parent)
    , m_mapData{deref(mapData)}
    , signaler{this}
    , lastEvent{ParseEvent::createDummyEvent()}
    , paths{PathList::alloc()}
{
    connect(&signaler,
            &RoomSignalHandler::sig_scheduleAction,
            this,
            &PathMachine::slot_scheduleAction);
}

void PathMachine::setCurrentRoom(const RoomId id, bool update)
{
    Forced forced(lastEvent, update);
    emit lookingForRooms(forced, id);
    releaseAllPaths();
    if (const Room *const perhaps = forced.oneMatch()) {
        setMostLikelyRoom(*perhaps);
        emit playerMoved(perhaps->getPosition());
        emit setCharPosition(perhaps->getId());
        state = PathStateEnum::APPROVED;
    } else {
        clearMostLikelyRoom();
        state = PathStateEnum::SYNCING;
    }
}

void PathMachine::releaseAllPaths()
{
    for (auto &path : *paths) {
        path->deny();
    }
    paths->clear();

    state = PathStateEnum::SYNCING;
}

void PathMachine::event(const SigParseEvent &sigParseEvent)
{
    if (lastEvent != sigParseEvent.requireValid())
        lastEvent = sigParseEvent;

    lastEvent.requireValid();

    switch (state) {
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
        emit lookingForRooms(recipient, room->getId());
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
        emit lookingForRooms(recipient, idx);
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
        emit lookingForRooms(recipient, c);

    } else {
        const Coordinate roomPos = room->getPosition();
        // REVISIT: Should this enumerate 6 or 7 values?
        // NOTE: This previously enumerated 8 values instead of 7,
        // which meant it was asking for exitDir(ExitDirEnum::NONE),
        // even though both ExitDirEnum::UNKNOWN and ExitDirEnum::NONE
        // both have Coordinate(0, 0, 0).
        for (const auto dir : ALL_EXITS7) {
            emit lookingForRooms(recipient, roomPos + Room::exitDir(dir));
        }
    }
}

void PathMachine::approved(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();

    Approved appr(sigParseEvent, params.matchingTolerance);
    const Room *perhaps = nullptr;

    if (event.getMoveType() == CommandEnum::LOOK) {
        emit lookingForRooms(appr, getMostLikelyRoomId());

    } else {
        tryExits(getMostLikelyRoom(), appr, event, true);
    }

    perhaps = appr.oneMatch();

    if (perhaps == nullptr) {
        // try to match by reverse exit
        appr.reset();
        tryExits(getMostLikelyRoom(), appr, event, false);
        perhaps = appr.oneMatch();
        if (perhaps == nullptr) {
            // try to match by coordinate
            appr.reset();
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
                    appr.reset();
                    Coordinate c = getMostLikelyRoomPosition() + eDir;
                    c.z--;
                    emit lookingForRooms(appr, c);
                    perhaps = appr.oneMatch();

                    if (perhaps == nullptr) {
                        // try to match by coordinate one step above expected
                        appr.reset();
                        c.z += 2;
                        emit lookingForRooms(appr, c);
                        perhaps = appr.oneMatch();
                    }
                }
            }
        }
    }

    if (perhaps == nullptr) {
        // couldn't match, give up
        state = PathStateEnum::EXPERIMENTING;
        m_pathRootPos = m_mostLikelyRoomPos;

        const Room *const pathRoot = getPathRoot();
        if (pathRoot == nullptr) {
            // What do we do now? "Who cares?"
            return;
        }

        paths->push_front(Path::alloc(pathRoot, nullptr, nullptr, &signaler, std::nullopt));
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
    if (bFlags.isValid()) {
        for (const auto dir : ALL_EXITS_NESWUD) {
            // guaranteed to succeed, since it's set above.
            const Exit &e = getMostLikelyRoom()->exit(dir);
            if (!e.outIsUnique()) {
                continue;
            }
            const RoomId connectedRoomId = e.outFirst();
            if (bFlags.hasDirectSunlight(dir)) {
                scheduleAction(std::make_shared<SingleRoomAction>(
                    std::make_unique<ModifyRoomFlags>(RoomSundeathEnum::SUNDEATH,
                                                      FlagModifyModeEnum::SET),
                    connectedRoomId));
            } else if (bFlags.hasNoDirectSunlight(dir)) {
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
    emit playerMoved(getMostLikelyRoomPosition());
    // GroupManager
    emit setCharPosition(getMostLikelyRoomId());
}

void PathMachine::syncing(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();
    {
        Syncing sync(params, paths, &signaler);
        if (event.getNumSkipped() <= params.maxSkipped) {
            emit lookingForRooms(sync, sigParseEvent);
        }
        paths = sync.evaluate();
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
        exp = std::make_unique<Crossover>(paths, dir, params);
        std::set<const Room *> pathEnds{};
        for (auto &path : *paths) {
            const Room *const working = path->getRoom();
            if (pathEnds.find(working) == pathEnds.end()) {
                emit createRoom(sigParseEvent, working->getPosition() + move);
                pathEnds.insert(working);
            }
        }
        emit lookingForRooms(*exp, sigParseEvent);
    } else {
        auto pOneByOne = std::make_unique<OneByOne>(sigParseEvent, params, &signaler);
        {
            auto &tmp = *pOneByOne;
            for (auto &path : *paths) {
                const Room *const working = path->getRoom();
                tmp.addPath(path);
                tryExits(working, tmp, event, true);
                tryExits(working, tmp, event, false);
                tryCoordinate(working, tmp, event);
            }
        }
        exp = static_upcast<Experimenting>(std::exchange(pOneByOne, nullptr));
    }

    paths = exp->evaluate();
    evaluatePaths();
}

void PathMachine::evaluatePaths()
{
    if (paths->empty()) {
        state = PathStateEnum::SYNCING;
    } else {
        if (const Room *const room = (paths->front()->getRoom()))
            setMostLikelyRoom(*room);
        else
            clearMostLikelyRoom();

        if (++paths->begin() == paths->end()) {
            state = PathStateEnum::APPROVED;
            paths->front()->approve();
            paths->pop_front();
        } else {
            state = PathStateEnum::EXPERIMENTING;
        }
        emit playerMoved(getMostLikelyRoomPosition());
        emit setCharPosition(getMostLikelyRoomId());
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
