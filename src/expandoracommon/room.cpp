// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "room.h"

#include <vector>

#include "../global/random.h"

ExitsList::ExitsList(const bool isDummy)
{
    for (auto &e : exits)
        e = Exit{!isDummy};
}

ExitDirConstRef::ExitDirConstRef(const ExitDirEnum dir, const Exit &exit)
    : dir{dir}
    , exit{exit}
{}

std::vector<ExitDirEnum> Room::getOutExits() const
{
    std::vector<ExitDirEnum> result;
    result.reserve(ALL_EXITS_NESWUD.size());
    for (auto dir : ALL_EXITS_NESWUD) {
        const Exit &e = this->exit(dir);
        if (e.isExit() && !e.outIsEmpty())
            result.emplace_back(dir);
    }
    return result;
}

OptionalExitDirConstRef Room::getRandomExit() const
{
    // Pick an alternative direction to randomly wander into
    const std::vector<ExitDirEnum> outExits = this->getOutExits();
    if (!outExits.empty()) {
        const auto randomDir = chooseRandomElement(outExits);
        return OptionalExitDirConstRef{ExitDirConstRef{randomDir, this->exit(randomDir)}};
    }

    return OptionalExitDirConstRef{};
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

RoomName Room::getName() const
{
    return fields.name;
}

RoomDescription Room::getStaticDescription() const
{
    return fields.staticDescription;
}

RoomDescription Room::getDynamicDescription() const
{
    return fields.dynamicDescription;
}

RoomNote Room::getNote() const
{
    return fields.note;
}

RoomMobFlags Room::getMobFlags() const
{
    return fields.mobFlags;
}

RoomLoadFlags Room::getLoadFlags() const
{
    return fields.loadFlags;
}

RoomTerrainEnum Room::getTerrainType() const
{
    return fields.terrainType;
}

RoomPortableEnum Room::getPortableType() const
{
    return fields.portableType;
}

RoomLightEnum Room::getLightType() const
{
    return fields.lightType;
}

RoomAlignEnum Room::getAlignType() const
{
    return fields.alignType;
}

RoomRidableEnum Room::getRidableType() const
{
    return fields.ridableType;
}

RoomSundeathEnum Room::getSundeathType() const
{
    return fields.sundeathType;
}

void Room::setName(RoomName value)
{
    fields.name = std::move(value);
}

void Room::setStaticDescription(RoomDescription value)
{
    fields.staticDescription = std::move(value);
}

void Room::setDynamicDescription(RoomDescription value)
{
    fields.dynamicDescription = std::move(value);
}

void Room::setNote(RoomNote value)
{
    fields.note = std::move(value);
}

void Room::setMobFlags(const RoomMobFlags value)
{
    fields.mobFlags = value;
}

void Room::setLoadFlags(const RoomLoadFlags value)
{
    fields.loadFlags = value;
}

void Room::setTerrainType(const RoomTerrainEnum value)
{
    fields.terrainType = value;
}

void Room::setPortableType(const RoomPortableEnum value)
{
    fields.portableType = value;
}

void Room::setLightType(const RoomLightEnum value)
{
    fields.lightType = value;
}

void Room::setAlignType(const RoomAlignEnum value)
{
    fields.alignType = value;
}

void Room::setRidableType(const RoomRidableEnum value)
{
    fields.ridableType = value;
}

void Room::setSundeathType(const RoomSundeathEnum value)
{
    fields.sundeathType = value;
}
