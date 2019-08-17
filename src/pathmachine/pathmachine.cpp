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
#include "../global/DirectionType.h"
#include "../global/roomid.h"
#include "../global/utils.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2room.h"
#include "../mapdata/roomfactory.h"
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

PathMachine::PathMachine(AbstractRoomFactory *const in_factory, QObject *const parent)
    : QObject(parent)
    , factory{in_factory}
    , signaler{this}
    , pathRoot{Room::tagDummy}
    , mostLikelyRoom{Room::tagDummy}
    , lastEvent{ParseEvent::createDummyEvent()}
    , paths{new PathList}
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
    if (const Room *perhaps = forced.oneMatch()) {
        // WARNING: This copies the current values of the room.
        mostLikelyRoom = *perhaps;
        emit playerMoved(mostLikelyRoom.getPosition());
        emit setCharPosition(mostLikelyRoom.getId());
        state = PathStateEnum::APPROVED;
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
                           ParseEvent &event,
                           const bool out)
{
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

void PathMachine::tryCoordinate(const Room *const room, RoomRecipient &recipient, ParseEvent &event)
{
    const CommandEnum moveCode = event.getMoveType();
    if (moveCode < CommandEnum::FLEE) {
        // LOOK, UNKNOWN will have an empty offset
        auto offset = RoomFactory::exitDir(getDirection(moveCode));
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
            emit lookingForRooms(recipient, roomPos + RoomFactory::exitDir(dir));
        }
    }
}

void PathMachine::approved(const SigParseEvent &sigParseEvent)
{
    ParseEvent &event = sigParseEvent.deref();

    Approved appr(factory, sigParseEvent, params.matchingTolerance);
    const Room *perhaps = nullptr;

    if (event.getMoveType() == CommandEnum::LOOK) {
        emit lookingForRooms(appr, mostLikelyRoom.getId());

    } else {
        tryExits(&mostLikelyRoom, appr, event, true);
    }

    perhaps = appr.oneMatch();

    if (perhaps == nullptr) {
        // try to match by reverse exit
        appr.reset();
        tryExits(&mostLikelyRoom, appr, event, false);
        perhaps = appr.oneMatch();
        if (perhaps == nullptr) {
            // try to match by coordinate
            appr.reset();
            tryCoordinate(&mostLikelyRoom, appr, event);
            perhaps = appr.oneMatch();
            if (perhaps == nullptr) {
                // try to match by coordinate one step below expected
                // FIXME: need stronger type checking here.

                const auto cmd = event.getMoveType();
                // NOTE: This allows ExitDirEnum::UNKNOWN,
                // which means the coordinate can be Coordinate(0,0,0).
                const Coordinate &eDir = RoomFactory::exitDir(getDirection(cmd));

                // CAUTION: This test seems to mean it wants only NESW,
                // but it would also accept ExitDirEnum::UNKNOWN,
                // which in the context of this function would mean "no move."
                if (eDir.z == 0) {
                    appr.reset();
                    Coordinate c = mostLikelyRoom.getPosition() + eDir;
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
        pathRoot = mostLikelyRoom;
        auto *const root = new Path(&pathRoot, nullptr, nullptr, &signaler);
        paths->push_front(root);
        experimenting(sigParseEvent);
        return;
    }

    // Update the exit from the previous room to the current room
    const CommandEnum move = event.getMoveType();
    if (static_cast<uint32_t>(move) < NUM_EXITS) {
        scheduleAction(std::make_unique<AddExit>(mostLikelyRoom.getId(),
                                                 perhaps->getId(),
                                                 static_cast<ExitDirEnum>(move)));
    }

    // Update most likely room with player's current location
    mostLikelyRoom = *perhaps;

    // Update rooms behind exits now that we are certain about our current location
    const ConnectedRoomFlagsType bFlags = event.getConnectedRoomFlags();
    if (bFlags.isValid()) {
        for (const auto dir : ALL_DIRECTIONS6) {
            const Exit &e = mostLikelyRoom.exit(dir);
            if (!e.outIsUnique()) {
                continue;
            }
            RoomId connectedRoomId = e.outFirst();
            auto bThisRoom = bFlags.getDirectionalLight(dir);
            if (IS_SET(bThisRoom, DirectionalLightEnum::DIRECT_SUN_ROOM)) {
                scheduleAction(std::make_unique<SingleRoomAction>(std::make_unique<UpdateRoomField>(
                                                                      RoomSundeathEnum::SUNDEATH),
                                                                  connectedRoomId));
            } else if (IS_SET(bThisRoom, DirectionalLightEnum::INDIRECT_SUN_ROOM)) {
                scheduleAction(std::make_unique<SingleRoomAction>(std::make_unique<UpdateRoomField>(
                                                                      RoomSundeathEnum::NO_SUNDEATH),
                                                                  connectedRoomId));
            }
        }
    }

    // Update the room if we had a tolerant match rather than an exact match
    if (appr.needsUpdate()) {
        emit scheduleAction(
            std::make_unique<SingleRoomAction>(std::make_unique<Update>(sigParseEvent),
                                               mostLikelyRoom.getId()));
    }

    // Send updates
    emit playerMoved(mostLikelyRoom.getPosition());
    // GroupManager
    emit setCharPosition(mostLikelyRoom.getId());
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
    const Coordinate &move = RoomFactory::exitDir(dir);

    // only create rooms if no properties are skipped and
    // the move coordinate is not 0,0,0

    if (event.getNumSkipped() == 0 && moveCode < CommandEnum::FLEE && !mostLikelyRoom.isFake()
        && !move.isNull()) {
        exp = std::make_unique<Crossover>(paths, dir, params, factory);
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
        auto pOneByOne = std::make_unique<OneByOne>(factory, sigParseEvent, params, &signaler);
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

void PathMachine::scheduleAction(const std::shared_ptr<MapAction> &action)
{
    emit sig_scheduleAction(action);
}

void PathMachine::evaluatePaths()
{
    if (paths->empty()) {
        state = PathStateEnum::SYNCING;
    } else {
        mostLikelyRoom = *(paths->front()->getRoom());
        if (++paths->begin() == paths->end()) {
            state = PathStateEnum::APPROVED;
            paths->front()->approve();
            paths->pop_front();
        } else {
            state = PathStateEnum::EXPERIMENTING;
        }
        emit playerMoved(mostLikelyRoom.getPosition());
        emit setCharPosition(mostLikelyRoom.getId());
    }
}
