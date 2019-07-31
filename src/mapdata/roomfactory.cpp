// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "roomfactory.h"

#include <QString>

#include "../expandoracommon/AbstractRoomFactory.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/exit.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/StringView.h"
#include "../parser/CommandId.h"
#include "../parser/ConnectedRoomFlags.h"
#include "../parser/ExitsFlags.h"
#include "../parser/PromptFlags.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFlags.h"
#include "mmapper2room.h"

Room *RoomFactory::createRoom() const
{
    return new Room(Room::tagValid);
}

Room *RoomFactory::createRoom(const ParseEvent &ev) const
{
    if (auto *const room = createRoom()) {
        update(*room, ev);
        return room;
    }

    /* This will never happen, because createRoom would throw std::bad_alloc(). */
    return nullptr;
}

SharedParseEvent RoomFactory::getEvent(const Room *const room) const
{
    ExitsFlagsType exitFlags;
    for (auto dir : ALL_EXITS_NESWUD) {
        const Exit &e = room->exit(dir);
        const ExitFlags eFlags = e.getExitFlags();
        exitFlags.set(dir, eFlags);
    }
    exitFlags.setValid();

    return ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                   room->getName(),
                                   room->getDynamicDescription(),
                                   room->getStaticDescription(),
                                   exitFlags,
                                   PromptFlagsType::fromRoomTerrainType(room->getTerrainType()),
                                   ConnectedRoomFlagsType{});
}

static int wordDifference(StringView a, StringView b)
{
    int diff = 0;
    while (!a.isEmpty() && !b.isEmpty())
        if (a.takeFirstLetter() != b.takeFirstLetter())
            ++diff;
    return diff + a.size() + b.size();
}

ComparisonResultEnum RoomFactory::compareStrings(const std::string &room,
                                                 const std::string &event,
                                                 int prevTolerance,
                                                 const bool updated)
{
    assert(prevTolerance >= 0);
    prevTolerance = std::max(0, prevTolerance);
    prevTolerance *= room.size();
    prevTolerance /= 100;
    int tolerance = prevTolerance;

    auto descWords = StringView{room}.trim();
    auto eventWords = StringView{event}.trim();

    if (!eventWords.isEmpty()) { // if event is empty we don't compare (due to blindness)
        while (tolerance >= 0) {
            if (descWords.isEmpty()) {
                if (updated) { // if notUpdated the desc is allowed to be shorter than the event
                    tolerance -= eventWords.countNonSpaceChars();
                }
                break;
            }
            if (eventWords.isEmpty()) { // if we get here the event isn't empty
                tolerance -= descWords.countNonSpaceChars();
                break;
            }

            tolerance -= wordDifference(eventWords.takeFirstWord(), descWords.takeFirstWord());
        }
    }

    if (tolerance < 0) {
        return ComparisonResultEnum::DIFFERENT;
    } else if (static_cast<int>(prevTolerance) != tolerance) {
        return ComparisonResultEnum::TOLERANCE;
    } else if (event.size() != room.size()) { // differences in amount of whitespace
        return ComparisonResultEnum::TOLERANCE;
    }
    return ComparisonResultEnum::EQUAL;
}

ComparisonResultEnum RoomFactory::compare(const Room *const room,
                                          const ParseEvent &event,
                                          const int tolerance) const
{
    const RoomName &name = room->getName();
    const RoomStaticDesc &staticDesc = room->getStaticDescription();
    const RoomTerrainEnum terrainType = room->getTerrainType();
    bool updated = room->isUpToDate();

    //    if (event == nullptr) {
    //        return ComparisonResultEnum::EQUAL;
    //    }

    if (name.isEmpty() && staticDesc.isEmpty() && (!updated)) {
        // user-created
        return ComparisonResultEnum::TOLERANCE;
    }

    const PromptFlagsType pf = event.getPromptFlags();
    if (pf.isValid()) {
        if (pf.getTerrainType() != terrainType) {
            if (room->isUpToDate()) {
                return ComparisonResultEnum::DIFFERENT;
            }
        }
    }

    switch (compareStrings(name.getStdString(), event.getRoomName().getStdString(), tolerance)) {
    case ComparisonResultEnum::TOLERANCE:
        updated = false;
        break;
    case ComparisonResultEnum::DIFFERENT:
        return ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::EQUAL:
        break;
    }

    switch (compareStrings(staticDesc.getStdString(),
                           event.getStaticDesc().getStdString(),
                           tolerance,
                           updated)) {
    case ComparisonResultEnum::TOLERANCE:
        updated = false;
        break;
    case ComparisonResultEnum::DIFFERENT:
        return ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::EQUAL:
        break;
    }

    switch (compareWeakProps(room, event, 0)) {
    case ComparisonResultEnum::DIFFERENT:
        return ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::TOLERANCE:
        updated = false;
        break;
    case ComparisonResultEnum::EQUAL:
        break;
    }

    if (updated) {
        return ComparisonResultEnum::EQUAL;
    }
    return ComparisonResultEnum::TOLERANCE;
}

ComparisonResultEnum RoomFactory::compareWeakProps(const Room *const room,
                                                   const ParseEvent &event,
                                                   int /*tolerance*/) const
{
    bool exitsValid = room->isUpToDate();
    // REVISIT: Should tolerance be an integer given known 'weak' params like hidden
    // exits or undefined flags?
    bool tolerance = false;

    const ConnectedRoomFlagsType connectedRoomFlags = event.getConnectedRoomFlags();
    const PromptFlagsType pFlags = event.getPromptFlags();
    if (pFlags.isValid()) {
        const RoomLightEnum lightType = room->getLightType();
        const RoomSundeathEnum sunType = room->getSundeathType();
        if (pFlags.isLit() && lightType != RoomLightEnum::LIT
            && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            // Allow prompt sunlight to override rooms without LIT flag if we know the room
            // is troll safe and obviously not in permanent darkness
            qDebug() << "Updating room to be LIT";
            tolerance = true;

        } else if (pFlags.isDark() && lightType != RoomLightEnum::DARK
                   && sunType == RoomSundeathEnum::NO_SUNDEATH && connectedRoomFlags.isValid()
                   && connectedRoomFlags.hasAnyDirectSunlight()) {
            // Allow prompt sunlight to override rooms without DARK flag if we know the room
            // has at least one sunlit exit and the room is troll safe
            qDebug() << "Updating room to be DARK";
            tolerance = true;
        }
    }

    const ExitsFlagsType eventExitsFlags = event.getExitsFlags();
    if (eventExitsFlags.isValid()) {
        bool previousDifference = false;
        for (auto dir : ALL_EXITS_NESWUD) {
            const Exit &roomExit = room->exit(dir);
            const ExitFlags roomExitFlags = roomExit.getExitFlags();
            if (roomExitFlags) {
                // exits are considered valid as soon as one exit is found (or if the room is updated)
                exitsValid = true;
                if (previousDifference) {
                    return ComparisonResultEnum::DIFFERENT;
                }
            }
            if (roomExitFlags.isNoMatch()) {
                continue;
            }
            const auto eventExitFlags = eventExitsFlags.get(dir);
            const auto diff = eventExitFlags ^ roomExitFlags;
            if (diff.isExit() || diff.isDoor()) {
                if (!exitsValid) {
                    // Room was not isUpToDate or the exits/doors do not match
                    previousDifference = true;
                } else {
                    if (tolerance) {
                        // Do not be tolerant for multiple differences
                        qDebug() << "Found too many differences" << event;
                        return ComparisonResultEnum::DIFFERENT;

                    } else if (!roomExitFlags.isExit() && eventExitFlags.isDoor()) {
                        // No exit exists on the map so we probably found a secret door
                        qDebug() << "Secret door likely found to the" << lowercaseDirection(dir)
                                 << event;
                        tolerance = true;

                    } else if (roomExit.isHiddenExit() && !eventExitFlags.isDoor()) {
                        qDebug() << "Secret exit hidden to the" << lowercaseDirection(dir);

                    } else {
                        qWarning() << "Unknown exit/door tolerance condition to the"
                                   << lowercaseDirection(dir) << event;
                        return ComparisonResultEnum::DIFFERENT;
                    }
                }
            } else if (diff.isRoad()) {
                if (roomExitFlags.isRoad() && connectedRoomFlags.isValid()
                    && connectedRoomFlags.hasDirectionalSunlight(static_cast<DirectionEnum>(dir))) {
                    // Orcs/trolls can only see trails/roads if it is dark (but can see climbs)
                    qDebug() << "Orc/troll could not see trail to the" << lowercaseDirection(dir);

                } else if (roomExitFlags.isRoad() && !eventExitFlags.isRoad()
                           && roomExitFlags.isDoor() && eventExitFlags.isDoor()) {
                    // A closed door is hiding the road that we know is there
                    qDebug() << "Closed door masking road/trail to the" << lowercaseDirection(dir);

                } else if (!roomExitFlags.isRoad() && eventExitFlags.isRoad()
                           && roomExitFlags.isDoor() && eventExitFlags.isDoor()) {
                    // A known door was previously mapped closed and a new road exit flag was found
                    qDebug() << "Previously closed door was hiding road to the"
                             << lowercaseDirection(dir);
                    tolerance = true;

                } else {
                    qWarning() << "Unknown road tolerance condition to the"
                               << lowercaseDirection(dir) << event;
                    tolerance = true;
                }
            } else if (diff.isClimb()) {
                if (roomExitFlags.isClimb() && !eventExitFlags.isClimb() && roomExitFlags.isDoor()
                    && eventExitFlags.isDoor()) {
                    // A closed door is hiding the climb that we know is there
                    qDebug() << "Closed door masking climb to the" << lowercaseDirection(dir);

                } else if (!roomExitFlags.isClimb() && eventExitFlags.isClimb()
                           && roomExitFlags.isDoor() && eventExitFlags.isDoor()) {
                    // A known door was previously mapped closed and a new climb exit flag was found
                    qDebug() << "Previously closed door was hiding climb to the"
                             << lowercaseDirection(dir);
                    tolerance = true;

                } else {
                    qWarning() << "Unknown climb tolerance condition to the"
                               << lowercaseDirection(dir) << event;
                    tolerance = true;
                }
            }
        }
    }
    if (tolerance || !exitsValid) {
        return ComparisonResultEnum::TOLERANCE;
    }
    return ComparisonResultEnum::EQUAL;
}

void RoomFactory::update(Room &room, const ParseEvent &event) const
{
    room.setDynamicDescription(event.getDynamicDesc());

    const ConnectedRoomFlagsType connectedRoomFlags = event.getConnectedRoomFlags();
    ExitsFlagsType eventExitsFlags = event.getExitsFlags();
    if (eventExitsFlags.isValid()) {
        eventExitsFlags.removeValid();
        for (const auto dir : ALL_EXITS_NESWUD) {
            ExitFlags eventExitFlags = eventExitsFlags.get(dir);
            Exit &roomExit = room.exit(dir);

            if (!room.isUpToDate()) {
                if (roomExit.isDoor() && !eventExitFlags.isDoor()) {
                    // Prevent room hidden exits from being overridden
                    eventExitFlags |= ExitFlagEnum::DOOR | ExitFlagEnum::EXIT;
                }
                if (roomExit.exitIsRoad() && !eventExitFlags.isRoad()
                    && connectedRoomFlags.isValid()
                    && connectedRoomFlags.hasDirectionalSunlight(static_cast<DirectionEnum>(dir))) {
                    // Prevent orcs/trolls from removing roads/trails if they're sunlit
                    eventExitFlags |= ExitFlagEnum::ROAD;
                }
                // Replace exits if target room is not up to date
                roomExit.setExitFlags(eventExitFlags);

            } else {
                // Update exits if target room is up to date
                roomExit.updateExit(eventExitFlags);
            }
        }
        room.setUpToDate();
    } else {
        room.setOutDated();
    }

    const PromptFlagsType pFlags = event.getPromptFlags();
    if (pFlags.isValid()) {
        room.setTerrainType(pFlags.getTerrainType());
        const RoomSundeathEnum sunType = room.getSundeathType();
        if (pFlags.isLit() && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            room.setLightType(RoomLightEnum::LIT);
        } else if (pFlags.isDark() && sunType == RoomSundeathEnum::NO_SUNDEATH
                   && connectedRoomFlags.isValid() && connectedRoomFlags.hasAnyDirectSunlight()) {
            room.setLightType(RoomLightEnum::DARK);
        }
    } else {
        room.setOutDated();
    }

    const RoomStaticDesc &desc = event.getStaticDesc();
    if (!desc.isEmpty()) {
        room.setStaticDescription(desc);
    } else {
        room.setOutDated();
    }

    const RoomName &name = event.getRoomName();
    if (!name.isEmpty()) {
        room.setName(name);
    } else {
        room.setOutDated();
    }
}

void RoomFactory::update(Room *const target, const Room *const source) const
{
    const RoomName name = source->getName();
    if (!name.isEmpty()) {
        target->setName(name);
    }
    const RoomStaticDesc desc = source->getStaticDescription();
    if (!desc.isEmpty()) {
        target->setStaticDescription(desc);
    }
    const RoomDynamicDesc dynamic = source->getDynamicDescription();
    if (!dynamic.isEmpty()) {
        target->setDynamicDescription(dynamic);
    }

    if (target->getAlignType() == RoomAlignEnum::UNDEFINED) {
        target->setAlignType(source->getAlignType());
    }
    if (target->getLightType() == RoomLightEnum::UNDEFINED) {
        target->setLightType(source->getLightType());
    }
    if (target->getSundeathType() == RoomSundeathEnum::UNDEFINED) {
        target->setSundeathType(source->getSundeathType());
    }
    if (target->getPortableType() == RoomPortableEnum::UNDEFINED) {
        target->setPortableType(source->getPortableType());
    }
    if (target->getRidableType() == RoomRidableEnum::UNDEFINED) {
        target->setRidableType(source->getRidableType());
    }
    if (source->getTerrainType() != RoomTerrainEnum::UNDEFINED) {
        target->setTerrainType(source->getTerrainType());
    }

    // REVISIT: why are these append operations, while the others replace?
    // REVISIT: And even if we accept appending, why is the target prepended?
    target->setNote(RoomNote{target->getNote().getStdString() + source->getNote().getStdString()});
    target->setMobFlags(target->getMobFlags() | source->getMobFlags());
    target->setLoadFlags(target->getLoadFlags() | source->getLoadFlags());

    if (!target->isUpToDate()) {
        // Replace data if target room is not up to date
        for (const auto dir : ALL_EXITS_NESWUD) {
            const Exit &sourceExit = source->exit(dir);
            Exit &targetExit = target->exit(dir);
            ExitFlags sourceExitFlags = sourceExit.getExitFlags();
            if (targetExit.isDoor()) {
                if (!sourceExitFlags.isDoor()) {
                    // Prevent target hidden exits from being overridden
                    sourceExitFlags |= ExitFlagEnum::DOOR | ExitFlagEnum::EXIT;
                } else {
                    targetExit.setDoorName(sourceExit.getDoorName());
                    targetExit.setDoorFlags(sourceExit.getDoorFlags());
                }
            }
            targetExit.setExitFlags(sourceExitFlags);
        }
    } else {
        // Combine data if target room is up to date
        for (const auto dir : ALL_EXITS_NESWUD) {
            const Exit &soureExit = source->exit(dir);
            Exit &targetExit = target->exit(dir);
            const ExitFlags sourceExitFlags = soureExit.getExitFlags();
            const ExitFlags targetExitFlags = targetExit.getExitFlags();
            if (targetExitFlags != sourceExitFlags) {
                targetExit.setExitFlags(targetExitFlags | sourceExitFlags);
            }
            const DoorName &sourceDoorName = soureExit.getDoorName();
            if (!sourceDoorName.isEmpty()) {
                targetExit.setDoorName(sourceDoorName);
            }
            const DoorFlags doorFlags = soureExit.getDoorFlags() | targetExit.getDoorFlags();
            targetExit.setDoorFlags(doorFlags);
        }
    }
    if (source->isUpToDate()) {
        target->setUpToDate();
    }
}

using ExitCoordinates = EnumIndexedArray<Coordinate, ExitDirEnum, NUM_EXITS_INCLUDING_NONE>;
static ExitCoordinates initExitCoordinates()
{
    // CAUTION: This choice of coordinate system will probably
    // come back to bite us if we ever try to go 3d.
    const Coordinate north(0, -1, 0);
    const Coordinate south(0, 1, 0); /* South is increasing Y */
    const Coordinate east(1, 0, 0);
    const Coordinate west(-1, 0, 0);
    const Coordinate up(0, 0, 1);
    const Coordinate down(0, 0, -1);

    ExitCoordinates exitDirs;
    exitDirs[ExitDirEnum::NORTH] = north;
    exitDirs[ExitDirEnum::SOUTH] = south;
    exitDirs[ExitDirEnum::EAST] = east;
    exitDirs[ExitDirEnum::WEST] = west;
    exitDirs[ExitDirEnum::UP] = up;
    exitDirs[ExitDirEnum::DOWN] = down;
    return exitDirs;
}

/* TODO: move this to another namespace */
const Coordinate &RoomFactory::exitDir(ExitDirEnum dir)
{
    static const auto exitDirs = initExitCoordinates();
    return exitDirs[dir];
}

RoomFactory::RoomFactory() = default;
