#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include <optional>
#include <QVariant>
#include <QVector>

#include "../global/DirectionType.h"
#include "../global/EnumIndexedArray.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"
#include "coordinate.h"
#include "exit.h"

// REVISIT: can't trivially make this
// `using ExitsList = EnumIndexedArray<Exit, ExitDirection, NUM_EXITS>`
// until we get rid of the concept of dummy exits and rooms,
// because the Exit still needs to be told if it has fields.
class ExitsList final
{
private:
    EnumIndexedArray<Exit, ExitDirection, NUM_EXITS> exits{};

public:
    explicit ExitsList(bool isDummy);

public:
    Exit &operator[](const ExitDirection idx) { return exits[idx]; }
    const Exit &operator[](const ExitDirection idx) const { return exits[idx]; }

public:
    auto size() const { return exits.size(); }

public:
    auto begin() { return exits.begin(); }
    auto end() { return exits.end(); }
    auto begin() const { return exits.begin(); }
    auto end() const { return exits.end(); }
    auto cbegin() const { return exits.cbegin(); }
    auto cend() const { return exits.cend(); }
};

struct ExitDirConstRef final
{
    ExitDirection dir;
    const Exit &exit;
    explicit ExitDirConstRef(const ExitDirection dir, const Exit &exit);
};

using OptionalExitDirConstRef = std::optional<ExitDirConstRef>;

class Room final
{
private:
    struct RoomFields final
    {
        RoomName name{};
        RoomDescription staticDescription{};
        RoomDescription dynamicDescription{};
        RoomNote note{};
        RoomMobFlags mobFlags{};
        RoomLoadFlags loadFlags{};
        RoomTerrainType terrainType{};
        RoomPortableType portableType{};
        RoomLightType lightType{};
        RoomAlignType alignType{};
        RoomRidableType ridableType{};
        RoomSundeathType sundeathType{};
    };

private:
    RoomId id = INVALID_ROOMID;
    Coordinate position{};
    bool temporary = true;
    bool upToDate = false;
    RoomFields fields{};
    ExitsList exits;
    bool isDummy_;

public:
    // TODO: merge DirectionType and ExitDirection enums
    Exit &exit(DirectionType dir) { return exits[static_cast<ExitDirection>(dir)]; }
    Exit &exit(ExitDirection dir) { return exits[dir]; }
    const Exit &exit(DirectionType dir) const { return exits[static_cast<ExitDirection>(dir)]; }
    const Exit &exit(ExitDirection dir) const { return exits[dir]; }

    ExitsList &getExitsList() { return exits; }
    const ExitsList &getExitsList() const { return exits; }

    std::vector<ExitDirection> getOutExits() const;
    OptionalExitDirConstRef getRandomExit() const;
    ExitDirConstRef getExitMaybeRandom(ExitDirection dir) const;

    void setId(const RoomId in) { id = in; }
    void setPosition(const Coordinate &in_c) { position = in_c; }
    RoomId getId() const { return id; }
    const Coordinate &getPosition() const { return position; }
    bool isTemporary() const
    {
        return temporary; // room is new if no exits are present
    }
    /* NOTE: This won't convert a "dummy" room to a valid room. */
    void setPermanent() { temporary = false; }
    bool isUpToDate() const { return upToDate; }
    void setUpToDate() { upToDate = true; }
    void setOutDated() { upToDate = false; }

public:
    RoomName getName() const;
    RoomDescription getStaticDescription() const;
    RoomDescription getDynamicDescription() const;
    RoomNote getNote() const;
    RoomMobFlags getMobFlags() const;
    RoomLoadFlags getLoadFlags() const;
    RoomTerrainType getTerrainType() const;
    RoomPortableType getPortableType() const;
    RoomLightType getLightType() const;
    RoomAlignType getAlignType() const;
    RoomRidableType getRidableType() const;
    RoomSundeathType getSundeathType() const;

    void setName(RoomName value);
    void setStaticDescription(RoomDescription value);
    void setDynamicDescription(RoomDescription value);
    void setNote(RoomNote value);
    void setMobFlags(RoomMobFlags value);
    void setLoadFlags(RoomLoadFlags value);
    void setTerrainType(RoomTerrainType value);
    void setPortableType(RoomPortableType value);
    void setLightType(RoomLightType value);
    void setAlignType(RoomAlignType value);
    void setRidableType(RoomRidableType value);
    void setSundeathType(RoomSundeathType value);

public:
    Room() = delete;
    static constexpr const struct TagDummy
    {
    } tagDummy{};
    static constexpr const struct TagValid
    {
    } tagValid{};
    explicit Room(TagDummy)
        : exits{true}
        , isDummy_{true}
    {
        assert(isFake());
    }
    explicit Room(TagValid)
        : exits{false}
        , isDummy_{false}
    {
        assert(!isFake());
    }

    ~Room() = default;

    // REVISIT: copies should be more explicit (e.g. room.copy(const Room& other)).
    DEFAULT_CTORS_AND_ASSIGN_OPS(Room);

public:
    bool isFake() const { return isDummy_; }
};
