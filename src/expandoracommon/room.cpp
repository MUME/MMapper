// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "room.h"

#include <memory>
#include <sstream>
#include <vector>

#include "../global/StringView.h"
#include "../global/random.h"
#include "../mapdata/ExitFieldVariant.h"
#include "parseevent.h"

static constexpr const auto default_updateFlags = RoomUpdateFlags{}; /* none */
static constexpr const auto mesh_updateFlags = RoomUpdateFlags{RoomUpdateEnum::Mesh};
static constexpr const auto key_updateFlags = RoomUpdateFlags{RoomUpdateEnum::NodeLookupKey};
static constexpr const auto borked_updateFlags = RoomUpdateFlags{RoomUpdateEnum::Borked}
                                                 | RoomUpdateEnum::Mesh;

static constexpr auto RoomName_updateFlags = key_updateFlags | RoomUpdateEnum::Name;
static constexpr auto RoomDesc_updateFlags = RoomUpdateFlags{RoomUpdateEnum::NodeLookupKey}
                                             | RoomUpdateEnum::Desc;
static constexpr auto RoomContents_updateFlags = RoomUpdateFlags{RoomUpdateEnum::NodeLookupKey}
                                                 | RoomUpdateEnum::Contents;
static constexpr auto RoomNote_updateFlags = RoomUpdateFlags{RoomUpdateEnum::Note};
static constexpr auto RoomMobFlags_updateFlags = mesh_updateFlags | RoomUpdateEnum::MobFlags;
static constexpr auto RoomLoadFlags_updateFlags = mesh_updateFlags | RoomUpdateEnum::LoadFlags;
static constexpr auto RoomTerrainEnum_updateFlags = mesh_updateFlags | key_updateFlags
                                                    | RoomUpdateEnum::Terrain;

static constexpr auto RoomPortableEnum_updateFlags = default_updateFlags;
static constexpr auto RoomLightEnum_updateFlags = mesh_updateFlags;
static constexpr auto RoomAlignEnum_updateFlags = default_updateFlags;
static constexpr auto RoomRidableEnum_updateFlags = mesh_updateFlags;
static constexpr auto RoomSundeathEnum_updateFlags = mesh_updateFlags;

static constexpr const auto doorNameUpdateFlags = mesh_updateFlags | RoomUpdateEnum::DoorName;
// REVISIT: Do these actually need to trigger a map update?
static constexpr const auto doorFlagUpdateFlags = mesh_updateFlags | RoomUpdateEnum::DoorFlags;
static constexpr const auto exitFlagUpdateFlags = mesh_updateFlags | key_updateFlags
                                                  | RoomUpdateEnum::ExitFlags;
static constexpr const auto incomingUpdateFlags = mesh_updateFlags | RoomUpdateEnum::ConnectionsIn;
static constexpr const auto outgoingUpdateFlags = mesh_updateFlags | key_updateFlags
                                                  | RoomUpdateEnum::ConnectionsOut;

RoomModificationTracker::~RoomModificationTracker() = default;

void RoomModificationTracker::notifyModified(Room &room, RoomUpdateFlags updateFlags)
{
    m_isModified = true;
    if (updateFlags.contains(RoomUpdateEnum::Mesh))
        m_needsMapUpdate = true;
    virt_onNotifyModified(room, updateFlags);
}

ExitDirConstRef::ExitDirConstRef(const ExitDirEnum dir, const Exit &exit)
    : dir{dir}
    , exit{exit}
{}

Room::Room(this_is_private, RoomModificationTracker &tracker, const RoomStatusEnum status)
    : m_tracker{tracker}
    , m_status{status}
{
    assert(status == RoomStatusEnum::Temporary || status == RoomStatusEnum::Permanent);
}

Room::~Room()
{
    // This fails for JsonWorld::writeZones
    if ((false))
        assert(m_status == RoomStatusEnum::Zombie);
}

ExitDirFlags Room::getOutExits() const
{
    ExitDirFlags result;
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const Exit &e = this->exit(dir);
        if (e.isExit() && !e.outIsEmpty())
            result |= dir;
    }
    return result;
}

OptionalExitDirConstRef Room::getRandomExit() const
{
    // Pick an alternative direction to randomly wander into
    const auto outExits = this->getOutExits();
    if (outExits.empty())
        return OptionalExitDirConstRef{};

    const auto randomDir = chooseRandomElement(outExits);
    return OptionalExitDirConstRef{ExitDirConstRef{randomDir, this->exit(randomDir)}};
}

ExitDirConstRef Room::getExitMaybeRandom(const ExitDirEnum dir) const
{
    // REVISIT: The whole room (not just exits) can be flagged as random in MUME.
    const Exit &e = this->exit(dir);

    if (e.exitIsRandom())
        if (auto opt = getRandomExit())
            return opt.value();

    return ExitDirConstRef{dir, e};
}

template<typename T>
inline bool maybeModify(T &ours, T &&value)
{
    if (ours == value)
        return false;

    ours = std::forward<T>(value);
    return true;
}

#define DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void Room::set##_Prop(_Type value) \
    { \
        if (maybeModify<_Type>((m_fields._Prop), std::move(value))) { \
            setModified(_Type##_updateFlags); \
        } \
    }

XFOREACH_ROOM_PROPERTY(DEFINE_SETTERS)
#undef DEFINE_SETTERS

#define DEFINE_SETTERS(_Type, _Prop, _OptInit) \
    void Room::set##_Type(ExitDirEnum dir, _Type value) \
    { \
        ExitsList exits = getExitsList(); \
        exits[dir].set##_Type(std::move(value)); \
        setExitsList(exits); \
    }

XFOREACH_EXIT_PROPERTY(DEFINE_SETTERS)
#undef DEFINE_SETTERS

void Room::setExitsList(const ExitsList &newExits)
{
    static const auto getDifferences = [](const Exit &a, const Exit &b) -> RoomUpdateFlags {
        RoomUpdateFlags flags;
        if (a.getDoorName() != b.getDoorName()) {
            flags |= doorNameUpdateFlags;
        }

        if (a.getDoorFlags() != b.getDoorFlags()) {
            flags |= doorFlagUpdateFlags;
        }

        if (a.getExitFlags() != b.getExitFlags()) {
            flags |= exitFlagUpdateFlags;
        }

        if (a.getIncoming() != b.getIncoming()) {
            flags |= incomingUpdateFlags;
        }

        if (a.getOutgoing() != b.getOutgoing()) {
            flags |= outgoingUpdateFlags;
        }
        return flags;
    };

    RoomUpdateFlags flags;

    for (const ExitDirEnum dir : ALL_EXITS7) {
        Exit &ex = m_exits[dir];
        const Exit &newValue = newExits[dir];
        if (ex == newValue)
            continue;

        const auto diff = getDifferences(ex, newValue);
        assert(!diff.empty());
        flags |= diff;
        ex = newValue;
        assert(ex == newValue);
    }

    if (!flags.empty())
        setModified(flags);
}

void Room::addInExit(const ExitDirEnum dir, const RoomId id)
{
    Exit &ex = exit(dir);
    if (ex.containsIn(id))
        return;
    ex.addIn(id);
    setModified(incomingUpdateFlags);
}

void Room::addOutExit(const ExitDirEnum dir, const RoomId id)
{
    Exit &ex = exit(dir);
    if (ex.containsOut(id))
        return;
    ex.addOut(id);
    setModified(outgoingUpdateFlags);
}

void Room::removeInExit(const ExitDirEnum dir, const RoomId id)
{
    Exit &ex = exit(dir);
    if (!ex.containsIn(id))
        return;
    ex.removeIn(id);
    setModified(incomingUpdateFlags);
}

void Room::removeOutExit(const ExitDirEnum dir, const RoomId id)
{
    Exit &ex = exit(dir);
    if (!ex.containsOut(id))
        return;
    // REVISIT: check if it was actually there?
    ex.removeOut(id);
    setModified(outgoingUpdateFlags);
}

void Room::setId(const RoomId id)
{
    if (m_id == id)
        return;

    m_id = id;
    setModified(RoomUpdateFlags{RoomUpdateEnum::Id});
}

void Room::setServerId(const RoomServerId &id)
{
    if (m_serverid == id)
        return;

    m_serverid = id;
    setModified(RoomUpdateFlags{RoomUpdateEnum::ServerId});
}

void Room::setPosition(const Coordinate &c)
{
    if (c == m_position)
        return;

    m_position = c;
    setModified(mesh_updateFlags | RoomUpdateEnum::Coord);
}

void Room::setPermanent()
{
    if (m_status == RoomStatusEnum::Zombie)
        throw std::runtime_error("Attempt to resurrect a zombie");

    const bool wasTemporary = std::exchange(m_status, RoomStatusEnum::Permanent)
                              == RoomStatusEnum::Temporary;
    if (wasTemporary)
        setModified(mesh_updateFlags);
}

void Room::setAboutToDie()
{
    // REVISIT: m_id = INVALID_ROOMID; ?
    m_status = RoomStatusEnum::Zombie;
}

void Room::setUpToDate()
{
    if (isUpToDate())
        return;

    m_borked = false;
    setModified(borked_updateFlags);
}

void Room::setOutDated()
{
    if (!isUpToDate())
        return;

    m_borked = true;
    setModified(borked_updateFlags);
}

void Room::setModified(const RoomUpdateFlags updateFlags)
{
    m_tracker.notifyModified(*this, updateFlags);
}

std::shared_ptr<Room> Room::createPermanentRoom(RoomModificationTracker &tracker)
{
    return std::make_shared<Room>(this_is_private{0}, tracker, RoomStatusEnum::Permanent);
}

SharedRoom Room::createTemporaryRoom(RoomModificationTracker &tracker, const ParseEvent &ev)
{
    auto room = std::make_shared<Room>(this_is_private{0}, tracker, RoomStatusEnum::Temporary);
    Room::update(*room, ev);
    return room;
}

SharedParseEvent Room::getEvent(const Room *const room)
{
    ExitsFlagsType exitFlags;
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const Exit &e = room->exit(dir);
        const ExitFlags eFlags = e.getExitFlags();
        exitFlags.set(dir, eFlags);
    }
    exitFlags.setValid();

    return ParseEvent::createEvent(CommandEnum::UNKNOWN,
                                   room->getServerId(),
                                   room->getName(),
                                   room->getDescription(),
                                   room->getContents(),
                                   room->getTerrainType(),
                                   exitFlags,
                                   PromptFlagsType{},
                                   ConnectedRoomFlagsType{});
}

NODISCARD static int wordDifference(StringView a, StringView b)
{
    size_t diff = 0;
    while (!a.isEmpty() && !b.isEmpty())
        if (a.takeFirstLetter() != b.takeFirstLetter())
            ++diff;
    return static_cast<int>(diff + a.size() + b.size());
}

ComparisonResultEnum Room::compareStrings(const std::string &room,
                                          const std::string &event,
                                          int prevTolerance,
                                          const bool updated)
{
    prevTolerance = utils::clampNonNegative(prevTolerance);
    prevTolerance *= static_cast<int>(room.size());
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
    } else if (prevTolerance != tolerance) {
        return ComparisonResultEnum::TOLERANCE;
    } else if (event.size() != room.size()) { // differences in amount of whitespace
        return ComparisonResultEnum::TOLERANCE;
    }
    return ComparisonResultEnum::EQUAL;
}

ComparisonResultEnum Room::compare(const Room *const room,
                                   const ParseEvent &event,
                                   const int tolerance)
{
    const auto &name = room->getName();
    const auto &desc = room->getDescription();
    const RoomTerrainEnum terrainType = room->getTerrainType();
    bool updated = room->isUpToDate();

    //    if (event == nullptr) {
    //        return ComparisonResultEnum::EQUAL;
    //    }

    if (name.isEmpty() && desc.isEmpty() && (!updated)) {
        // user-created
        return ComparisonResultEnum::TOLERANCE;
    }

    if (event.getTerrainType() != terrainType && room->isUpToDate()) {
        return ComparisonResultEnum::DIFFERENT;
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

    switch (
        compareStrings(desc.getStdString(), event.getRoomDesc().getStdString(), tolerance, updated)) {
    case ComparisonResultEnum::TOLERANCE:
        updated = false;
        break;
    case ComparisonResultEnum::DIFFERENT:
        return ComparisonResultEnum::DIFFERENT;
    case ComparisonResultEnum::EQUAL:
        break;
    }

    switch (compareWeakProps(room, event)) {
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

ComparisonResultEnum Room::compareWeakProps(const Room *const room, const ParseEvent &event)
{
    bool exitsValid = room->isUpToDate();
    // REVISIT: Should tolerance be an integer given known 'weak' params like hidden
    // exits or undefined flags?
    bool tolerance = false;

    const ConnectedRoomFlagsType connectedRoomFlags = event.getConnectedRoomFlags();
    const PromptFlagsType pFlags = event.getPromptFlags();
    if (pFlags.isValid() && connectedRoomFlags.isValid() && connectedRoomFlags.isTrollMode()) {
        const RoomLightEnum lightType = room->getLightType();
        const RoomSundeathEnum sunType = room->getSundeathType();
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
                        qDebug() << "Found too many differences" << event << *room;
                        return ComparisonResultEnum::DIFFERENT;

                    } else if (!roomExitFlags.isExit() && eventExitFlags.isDoor()) {
                        // No exit exists on the map so we probably found a secret door
                        qDebug() << "Secret door likely found to the" << lowercaseDirection(dir)
                                 << event;
                        tolerance = true;

                    } else if (roomExit.isHiddenExit() && !eventExitFlags.isDoor()) {
                        qDebug() << "Secret exit hidden to the" << lowercaseDirection(dir);

                    } else if (roomExitFlags.isExit() && roomExitFlags.isDoor()
                               && !eventExitFlags.isExit()) {
                        qDebug() << "Door to the" << lowercaseDirection(dir)
                                 << "is likely a secret";
                        tolerance = true;

                    } else {
                        qWarning() << "Unknown exit/door tolerance condition to the"
                                   << lowercaseDirection(dir) << event << *room;
                        return ComparisonResultEnum::DIFFERENT;
                    }
                }
            } else if (diff.isRoad()) {
                if (roomExitFlags.isRoad() && hasLight) {
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
                               << lowercaseDirection(dir) << event << *room;
                    tolerance = true;
                }
            } else if (diff.isClimb()) {
                if (roomExitFlags.isDoor() && roomExitFlags.isClimb()) {
                    // A closed door is hiding the climb that we know is there
                    qDebug() << "Door masking climb to the" << lowercaseDirection(dir);

                } else {
                    qWarning() << "Unknown climb tolerance condition to the"
                               << lowercaseDirection(dir) << event << *room;
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

void Room::update(Room &room, const ParseEvent &event)
{
    room.setContents(event.getRoomContents());
    bool isUpToDate = room.isUpToDate();

    const ConnectedRoomFlagsType connectedRoomFlags = event.getConnectedRoomFlags();
    ExitsFlagsType eventExitsFlags = event.getExitsFlags();
    if (!eventExitsFlags.isValid()) {
        isUpToDate = false;
    } else {
        eventExitsFlags.removeValid();
        ExitsList copiedExits = room.getExitsList();
        if (room.isUpToDate()) {
            // Append exit flags if target room is up to date
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                Exit &roomExit = copiedExits[dir];
                const ExitFlags &roomExitFlags = roomExit.getExitFlags();
                const ExitFlags &eventExitFlags = eventExitsFlags.get(dir);
                if (eventExitFlags ^ roomExitFlags) {
                    roomExit.setExitFlags(roomExitFlags | eventExitFlags);
                }
            }

        } else {
            // Replace exit flags if target room is not up to date
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                Exit &roomExit = copiedExits[dir];
                ExitFlags eventExitFlags = eventExitsFlags.get(dir);
                // ... but take care of the following exceptions
                if (roomExit.isDoor() && !eventExitFlags.isDoor()) {
                    // Prevent room hidden exits from being overridden
                    eventExitFlags |= ExitFlagEnum::DOOR | ExitFlagEnum::EXIT;
                }
                if (roomExit.exitIsRoad() && !eventExitFlags.isRoad()
                    && connectedRoomFlags.isValid() && connectedRoomFlags.hasDirectSunlight(dir)) {
                    // Prevent orcs/trolls from removing roads/trails if they're sunlit
                    eventExitFlags |= ExitFlagEnum::ROAD;
                }
                roomExit.setExitFlags(eventExitFlags);
            }
        }
        room.setExitsList(copiedExits);
        isUpToDate = true;
    }

    const PromptFlagsType pFlags = event.getPromptFlags();
    if (pFlags.isValid() && connectedRoomFlags.isValid() && connectedRoomFlags.isTrollMode()) {
        const RoomSundeathEnum sunType = room.getSundeathType();
        if (pFlags.isLit() && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            room.setLightType(RoomLightEnum::LIT);
        } else if (pFlags.isDark() && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            room.setLightType(RoomLightEnum::DARK);
        }
    }

    const auto &serverId = event.getRoomServerId();
    if (!serverId.isSet()) {
        isUpToDate = false;
    } else {
        room.setServerId(serverId);
    }

    const auto &terrain = event.getTerrainType();
    if (terrain == RoomTerrainEnum::UNDEFINED) {
        isUpToDate = false;
    } else {
        room.setTerrainType(terrain);
    }

    const auto &desc = event.getRoomDesc();
    if (desc.isEmpty()) {
        isUpToDate = false;
    } else {
        room.setDescription(desc);
    }

    const auto &name = event.getRoomName();
    if (name.isEmpty()) {
        isUpToDate = false;
    } else {
        room.setName(name);
    }

    if (isUpToDate)
        room.setUpToDate();
    else
        room.setOutDated();
}

void Room::update(Room *const target, const Room *const source)
{
    const auto &serverId = source->getServerId();
    if (serverId.isSet()) {
        target->setServerId(serverId);
    }
    const auto &name = source->getName();
    if (!name.isEmpty()) {
        target->setName(name);
    }
    const auto &desc = source->getDescription();
    if (!desc.isEmpty()) {
        target->setDescription(desc);
    }
    const auto &contents = source->getContents();
    if (!contents.isEmpty()) {
        target->setContents(contents);
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
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
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
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
            const Exit &soureExit = source->exit(dir);
            Exit &targetExit = target->exit(dir);
            const ExitFlags sourceExitFlags = soureExit.getExitFlags();
            const ExitFlags targetExitFlags = targetExit.getExitFlags();
            if (targetExitFlags != sourceExitFlags) {
                targetExit.setExitFlags(targetExitFlags | sourceExitFlags);
            }
            const auto &sourceDoorName = soureExit.getDoorName();
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

std::string Room::toStdString() const
{
    std::stringstream ss;
    ss << getName().getStdString() << "\n"
       << getDescription().getStdString() << getContents().getStdString();

    ss << "Exits:";
    for (const ExitDirEnum j : ALL_EXITS7) {
        const ExitFlags &exitFlags = exit(j).getExitFlags();
        if (!exitFlags.isExit())
            continue;
        ss << " ";

        bool climb = exit(j).getExitFlags().isClimb();
        if (climb)
            ss << "|";
        bool door = exit(j).isDoor();
        if (door)
            ss << "(";
        ss << lowercaseDirection(j);
        if (door) {
            const auto doorName = exit(j).getDoorName();
            if (!doorName.isEmpty()) {
                ss << "/" << doorName.getStdString();
            }
            ss << ")";
        }
        if (climb)
            ss << "|";
    }
    ss << ".\n";
    if (!getNote().isEmpty()) {
        ss << "Note: " << getNote().getStdString();
    }

    return ss.str();
}

using ExitCoordinates = EnumIndexedArray<Coordinate, ExitDirEnum, NUM_EXITS_INCLUDING_NONE>;
NODISCARD static ExitCoordinates initExitCoordinates()
{
    ExitCoordinates exitDirs;
    exitDirs[ExitDirEnum::NORTH] = Coordinate(0, 1, 0);
    exitDirs[ExitDirEnum::SOUTH] = Coordinate(0, -1, 0);
    exitDirs[ExitDirEnum::EAST] = Coordinate(1, 0, 0);
    exitDirs[ExitDirEnum::WEST] = Coordinate(-1, 0, 0);
    exitDirs[ExitDirEnum::UP] = Coordinate(0, 0, 1);
    exitDirs[ExitDirEnum::DOWN] = Coordinate(0, 0, -1);
    return exitDirs;
}

const Coordinate &Room::exitDir(ExitDirEnum dir)
{
    static const auto exitDirs = initExitCoordinates();
    return exitDirs[dir];
}

std::shared_ptr<Room> Room::clone(RoomModificationTracker &tracker) const
{
    if (m_status == RoomStatusEnum::Zombie)
        throw std::runtime_error("Attempt to clone a zombie");

    const auto copy = std::make_shared<Room>(this_is_private{0}, tracker, RoomStatusEnum::Temporary);
#define COPY(x) \
    do { \
        copy->x = this->x; \
    } while (false)
    COPY(m_position);
    COPY(m_fields);
    COPY(m_exits);
    COPY(m_id);
    COPY(m_serverid);
    COPY(m_status);
    COPY(m_borked);
#undef COPY
    switch (copy->m_status) {
    case RoomStatusEnum::Permanent:
        copy->m_status = RoomStatusEnum::Temporary;
        break;
    case RoomStatusEnum::Temporary:
    case RoomStatusEnum::Zombie:
        // no change
        break;
    }
    return copy;
}
