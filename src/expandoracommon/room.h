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

#ifndef ROOM_H
#define ROOM_H

#include "../global/DirectionType.h"
#include "../global/EnumIndexedArray.h"
#include "../global/roomid.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"
#include "coordinate.h"
#include "exit.h"
#include <QVariant>
#include <QVector>

// REVISIT: can't trivially make this
// `using ExitsList = EnumIndexedArray<Exit, ExitDirection, NUM_EXITS>`
// until we get rid of the concept of dummy exits and rooms,
// because the Exit still needs to be told if it has fields.
class ExitsList final
{
private:
    EnumIndexedArray<Exit, ExitDirection, NUM_EXITS> exits{};

public:
    explicit ExitsList(const bool isDummy)
    {
        for (auto &e : exits)
            e = Exit{!isDummy};
    }

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
    [[deprecated]] uint32_t at(const RoomField field) const {
#define CASE_INT(UPPER, camelCase) \
    do { \
    case RoomField::UPPER: \
        return static_cast<uint32_t>(fields.camelCase); \
    } while (false)
        switch (field) {
            CASE_INT(TERRAIN_TYPE, terrainType);
            CASE_INT(MOB_FLAGS, mobFlags);
            CASE_INT(LOAD_FLAGS, loadFlags);
            CASE_INT(PORTABLE_TYPE, portableType);
            CASE_INT(LIGHT_TYPE, lightType);
            CASE_INT(ALIGN_TYPE, alignType);
            CASE_INT(RIDABLE_TYPE, ridableType);
            CASE_INT(SUNDEATH_TYPE, sundeathType);
        case RoomField::NAME:
        case RoomField::DESC:
        case RoomField::DYNAMIC_DESC:
        case RoomField::NOTE:
        case RoomField::RESERVED:
        case RoomField::LAST:
            break;
        }
        throw std::invalid_argument("type");
#undef CASE_INT
    }
        //
        [[deprecated]] void replace(const RoomField field, QVariant value)
    {
#define CASE_STR(UPPER, camelCase) \
    do { \
    case RoomField::UPPER: \
        fields.camelCase = value.toString(); \
        return; \
    } while (false)
#define CASE_INT(UPPER, camelCase) \
    do { \
    case RoomField::UPPER: \
        fields.camelCase = static_cast<decltype(fields.camelCase)>(value.toUInt()); \
        return; \
    } while (false)
        switch (field) {
            CASE_STR(NAME, name);
            CASE_STR(DESC, staticDescription);
            CASE_STR(DYNAMIC_DESC, dynamicDescription);
            CASE_STR(NOTE, note);
            CASE_INT(TERRAIN_TYPE, terrainType);
            CASE_INT(MOB_FLAGS, mobFlags);
            CASE_INT(LOAD_FLAGS, loadFlags);
            CASE_INT(PORTABLE_TYPE, portableType);
            CASE_INT(LIGHT_TYPE, lightType);
            CASE_INT(ALIGN_TYPE, alignType);
            CASE_INT(RIDABLE_TYPE, ridableType);
            CASE_INT(SUNDEATH_TYPE, sundeathType);
        case RoomField::RESERVED:
        case RoomField::LAST:
            break;
        }
        throw std::invalid_argument("type");
#undef CASE_STR
#undef CASE_INT
    }

public:
    template<typename Callback>
    [[deprecated]] inline void modifyUint(const RoomField field, Callback &&callback)
    {
        const uint oldValue = at(field);
        replace(field, callback(oldValue));
    }

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
    Room(Room &&) = default;
    Room(const Room &) = default;
    Room &operator=(Room &&) = default;
    Room &operator=(const Room &) = default;

public:
    bool isFake() const { return isDummy_; }
};

#endif
