// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "room.h"

#include "../global/StringView.h"
#include "Compare.h"
#include "RoomHandle.h"
#include "parseevent.h"

#include <sstream>
#include <vector>

namespace { // anonymous
static volatile bool spam_and_lag = false;
}

RoomModificationTracker::~RoomModificationTracker() = default;

void RoomModificationTracker::notifyModified(const RoomUpdateFlags updateFlags)
{
    if (!updateFlags.empty()) {
        m_needsMapUpdate = true;
    }
    virt_onNotifyModified(updateFlags);
}

NODISCARD static int wordDifference(StringView a, StringView b)
{
    size_t diff = 0;
    while (!a.isEmpty() && !b.isEmpty()) {
        if (a.takeFirstLetter() != b.takeFirstLetter()) {
            ++diff;
        }
    }
    return static_cast<int>(diff + a.size() + b.size());
}

ComparisonResultEnum compareStrings(const std::string_view room,
                                    const std::string_view event,
                                    int prevTolerance,
                                    const bool upToDate)
{
    prevTolerance = utils::clampNonNegative(prevTolerance);
    prevTolerance *= static_cast<int>(room.size());
    prevTolerance /= 100;
    int tolerance = prevTolerance;

    auto descWords = StringView{room}.trim();
    auto eventWords = StringView{event}.trim();

    if (!eventWords.isEmpty()) {
        // if event is empty we don't compare (due to blindness)
        while (tolerance >= 0) {
            if (descWords.isEmpty()) {
                if (upToDate) {
                    // the desc is allowed to be shorter than the event
                    tolerance -= eventWords.countNonSpaceChars();
                }
                break;
            }
            if (eventWords.isEmpty()) {
                // if we get here the event isn't empty
                tolerance -= descWords.countNonSpaceChars();
                break;
            }

            tolerance -= wordDifference(eventWords.takeFirstWord(), descWords.takeFirstWord());
        }
    }

    if (tolerance < 0) {
        return ComparisonResultEnum::DIFFERENT;
    } else if (prevTolerance != tolerance) {
        return ComparisonResultEnum::TOLERANCE;
    } else if (event.size() != room.size()) {
        // differences in amount of whitespace
        return ComparisonResultEnum::TOLERANCE;
    }
    return ComparisonResultEnum::EQUAL;
}

ComparisonResultEnum compare(const RawRoom &room, const ParseEvent &event, const int tolerance)
{
    const auto &name = room.getName();
    const auto &desc = room.getDescription();
    const RoomTerrainEnum terrainType = room.getTerrainType();
    bool mapIdMatch = false;
    bool upToDate = true;

    //    if (event == nullptr) {
    //        return ComparisonResultEnum::EQUAL;
    //    }

    if (name.isEmpty() && desc.isEmpty() && terrainType == RoomTerrainEnum::UNDEFINED) {
        // user-created
        return ComparisonResultEnum::TOLERANCE;
    }

    if (event.getServerId() == INVALID_SERVER_ROOMID // fog/darkness results in no MapId
        || room.getServerId() == INVALID_SERVER_ROOMID) {
        mapIdMatch = false;
    } else if (event.getServerId() == room.getServerId()) {
        mapIdMatch = true;
    } else {
        return ComparisonResultEnum::DIFFERENT;
    }

    if (event.getTerrainType() != terrainType) {
        return mapIdMatch ? ComparisonResultEnum::TOLERANCE : ComparisonResultEnum::DIFFERENT;
    }

    switch (compareStrings(name.getStdStringViewUtf8(),
                           event.getRoomName().getStdStringViewUtf8(),
                           tolerance)) {
    case ComparisonResultEnum::DIFFERENT:
        return mapIdMatch ? ComparisonResultEnum::TOLERANCE : ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::EQUAL:
        break;
    case ComparisonResultEnum::TOLERANCE:
        upToDate = false;
        break;
    }

    switch (compareStrings(desc.getStdStringViewUtf8(),
                           event.getRoomDesc().getStdStringViewUtf8(),
                           tolerance,
                           upToDate)) {
    case ComparisonResultEnum::DIFFERENT:
        return mapIdMatch ? ComparisonResultEnum::TOLERANCE : ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::EQUAL:
        break;
    case ComparisonResultEnum::TOLERANCE:
        upToDate = false;
        break;
    }

    switch (compareWeakProps(room, event)) {
    case ComparisonResultEnum::DIFFERENT:
        return mapIdMatch ? ComparisonResultEnum::TOLERANCE : ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::EQUAL:
        break;
    case ComparisonResultEnum::TOLERANCE:
        upToDate = false;
        break;
    }

    if (upToDate && event.hasServerId() && !mapIdMatch) {
        // room is missing server id
        upToDate = false;
    }

    if (upToDate && room.getArea() != event.getRoomArea()) {
        // room is missing area
        upToDate = false;
    }

    if (upToDate) {
        return ComparisonResultEnum::EQUAL;
    }
    return ComparisonResultEnum::TOLERANCE;
}

ComparisonResultEnum compareWeakProps(const RawRoom &room, const ParseEvent &event)
{
    bool exitsValid = true;
    // REVISIT: Should tolerance be an integer given known 'weak' params like hidden
    // exits or undefined flags?
    bool tolerance = false;

    const ConnectedRoomFlagsType connectedRoomFlags = event.getConnectedRoomFlags();
    const PromptFlagsType pFlags = event.getPromptFlags();
    if (pFlags.isValid() && connectedRoomFlags.isValid() && connectedRoomFlags.isTrollMode()) {
        const RoomLightEnum lightType = room.getLightType();
        const RoomSundeathEnum sunType = room.getSundeathType();
        if (pFlags.isLit() && lightType != RoomLightEnum::LIT
            && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            // Allow prompt sunlight to override rooms without LIT flag if we know the room
            // is troll safe and obviously not in permanent darkness
            qDebug() << "Updating room to be LIT";
            tolerance = true;

        } else if (pFlags.isDark() && lightType != RoomLightEnum::DARK
                   && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            // Allow prompt sunlight to override rooms without DARK flag if we know the room
            // has at least one sunlit exit and the room is troll safe
            qDebug() << "Updating room to be DARK";
            tolerance = true;
        }
    }

    const ExitsFlagsType eventExitsFlags = event.getExitsFlags();
    if (eventExitsFlags.isValid()) {
        bool previousDifference = false;
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
            const auto &roomExit = room.getExit(dir);
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
            const bool hasLight = connectedRoomFlags.isValid()
                                  && connectedRoomFlags.hasDirectSunlight(dir);
            const auto eventExitFlags = eventExitsFlags.get(dir);
            const auto diff = eventExitFlags ^ roomExitFlags;
            /* MUME has two logic flows for displaying signs on exits:
             *
             * 1) Display one sign for a portal {} or closed door []
             *    i.e. {North} [South]
             *
             * 2) Display two signs from each list in the following order:
             *    a) one option of: * ^ = - ~
             *    b) one option of: open door () or climb up /\ or climb down \/
             *    i.e. *(North)* -/South\- ~East~ *West*
             *
             * You can combine the two flows for each exit: {North} ~East~ *(West)*
            */
            if (diff.isExit() || diff.isDoor()) {
                if (!exitsValid) {
                    // Room was not isUpToDate and no exits were present in the room
                    previousDifference = true;
                } else {
                    if (tolerance) {
                        // Do not be tolerant for multiple differences
                        if (spam_and_lag) {
                            qDebug() << "Found too many differences" << event
                                     << mmqt::toQStringUtf8(room.toStdStringUtf8());
                        }
                        return ComparisonResultEnum::DIFFERENT;

                    } else if (!roomExitFlags.isExit() && eventExitFlags.isDoor()) {
                        // No exit exists on the map so we probably found a secret door
                        if (spam_and_lag) {
                            qDebug() << "Secret door likely found to the" << lowercaseDirection(dir)
                                     << event;
                        }
                        tolerance = true;

                    } else if (roomExit.doorIsHidden() && !eventExitFlags.isDoor()) {
                        if (spam_and_lag) {
                            qDebug() << "Secret exit hidden to the" << lowercaseDirection(dir);
                        }
                    } else if (roomExitFlags.isExit() && roomExitFlags.isDoor()
                               && !eventExitFlags.isExit()) {
                        if (spam_and_lag) {
                            qDebug()
                                << "Door to the" << lowercaseDirection(dir) << "is likely a secret";
                        }
                        tolerance = true;

                    } else {
                        if (spam_and_lag) {
                            qWarning() << "Unknown exit/door tolerance condition to the"
                                       << lowercaseDirection(dir) << event
                                       << mmqt::toQStringUtf8(room.toStdStringUtf8());
                        }
                        return ComparisonResultEnum::DIFFERENT;
                    }
                }
            } else if (diff.isRoad()) {
                if (roomExitFlags.isRoad() && hasLight) {
                    // Orcs/trolls can only see trails/roads if it is dark (but can see climbs)
                    if (spam_and_lag) {
                        qDebug() << "Orc/troll could not see trail to the"
                                 << lowercaseDirection(dir);
                    }

                } else if (roomExitFlags.isRoad() && !eventExitFlags.isRoad()
                           && roomExitFlags.isDoor() && eventExitFlags.isDoor()) {
                    // A closed door is hiding the road that we know is there
                    if (spam_and_lag) {
                        qDebug() << "Closed door masking road/trail to the"
                                 << lowercaseDirection(dir);
                    }

                } else if (!roomExitFlags.isRoad() && eventExitFlags.isRoad()
                           && roomExitFlags.isDoor() && eventExitFlags.isDoor()) {
                    // A known door was previously mapped closed and a new road exit flag was found
                    if (spam_and_lag) {
                        qDebug() << "Previously closed door was hiding road to the"
                                 << lowercaseDirection(dir);
                    }
                    tolerance = true;

                } else {
                    if (spam_and_lag) {
                        qWarning()
                            << "Unknown road tolerance condition to the" << lowercaseDirection(dir)
                            << event << mmqt::toQStringUtf8(room.toStdStringUtf8());
                    }
                    // TODO: Likely an old road/trail that needs to be removed
                    tolerance = true;
                }
            } else if (diff.isClimb()) {
                if (roomExitFlags.isDoor() && roomExitFlags.isClimb()) {
                    // A closed door is hiding the climb that we know is there
                    if (spam_and_lag) {
                        qDebug() << "Door masking climb to the" << lowercaseDirection(dir);
                    }

                } else {
                    if (spam_and_lag) {
                        qWarning()
                            << "Unknown climb tolerance condition to the" << lowercaseDirection(dir)
                            << event << mmqt::toQStringUtf8(room.toStdStringUtf8());
                    }
                    // TODO: Likely an old climb that needs to be removed
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
