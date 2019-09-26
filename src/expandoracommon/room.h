#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cassert>
#include <memory>
#include <optional>
#include <QVariant>
#include <QVector>

#include "../global/DirectionType.h"
#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"
#include "coordinate.h"
#include "exit.h"

class ExitFieldVariant;
class ParseEvent;

enum class FlagModifyModeEnum { SET, UNSET, TOGGLE };
enum class ComparisonResultEnum { DIFFERENT = 0, EQUAL, TOLERANCE };

template<typename Flags, typename Flag>
static inline Flags modifyFlags(const Flags flags, const Flag x, const FlagModifyModeEnum mode)
{
    switch (mode) {
    case FlagModifyModeEnum::SET:
        return flags | x;
    case FlagModifyModeEnum::UNSET:
        return flags & (~x);
    case FlagModifyModeEnum::TOGGLE:
        return flags ^ x;
    }
    // REVISIT: convert to std::abort() ?
    assert(false);
    return flags;
}

// REVISIT: can't trivially make this
// `using ExitsList = EnumIndexedArray<Exit, ExitDirEnum, NUM_EXITS>`
// until we get rid of the concept of dummy exits and rooms,
// because the Exit still needs to be told if it has fields.
class ExitsList final
{
private:
    EnumIndexedArray<Exit, ExitDirEnum, NUM_EXITS> m_exits;

public:
    Exit &operator[](const ExitDirEnum idx) { return m_exits[idx]; }
    const Exit &operator[](const ExitDirEnum idx) const { return m_exits[idx]; }

public:
    auto size() const { return m_exits.size(); }

public:
    auto begin() { return m_exits.begin(); }
    auto end() { return m_exits.end(); }
    auto begin() const { return m_exits.begin(); }
    auto end() const { return m_exits.end(); }
    auto cbegin() const { return m_exits.cbegin(); }
    auto cend() const { return m_exits.cend(); }
};

struct ExitDirConstRef final
{
    const ExitDirEnum dir;
    const Exit &exit;
    explicit ExitDirConstRef(ExitDirEnum dir, const Exit &exit);
};

using OptionalExitDirConstRef = std::optional<ExitDirConstRef>;

class Room;

enum class RoomUpdateEnum {
    Id,
    Coord,
    NodeLookupKey,

    Mesh,
    ConnectionsIn,
    ConnectionsOut,

    Name,
    StaticDesc,
    DynamicDesc,
    Note,
    Terrain,

    DoorFlags,
    DoorName,
    ExitFlags,
    LoadFlags,
    MobFlags,

    Borked,
};

static constexpr const size_t NUM_ROOM_UPDATE_TYPES = 17;
static_assert(NUM_ROOM_UPDATE_TYPES == static_cast<int>(RoomUpdateEnum::Borked) + 1);
DEFINE_ENUM_COUNT(RoomUpdateEnum, NUM_ROOM_UPDATE_TYPES)

struct RoomUpdateFlags final : enums::Flags<RoomUpdateFlags, RoomUpdateEnum, uint32_t>
{
    using Flags::Flags;
};

class RoomModificationTracker
{
private:
    bool m_isModified = false;
    bool m_needsMapUpdate = false;

public:
    virtual ~RoomModificationTracker();

public:
    void notifyModified(Room &room, RoomUpdateFlags updateFlags);
    virtual void virt_onNotifyModified(Room & /*room*/, RoomUpdateFlags /*updateFlags*/) {}

public:
    bool isModified() const { return m_isModified; }
    void clearModified() { m_isModified = false; }

public:
    bool getNeedsMapUpdate() const { return m_needsMapUpdate; }
    void clearNeedsMapUpdate() { m_needsMapUpdate = false; }
};

// NOTE: Names are capitalized for use with getRoomName() and setRoomName(),
// which means they'll be capitalized as RoomFields::RoomName.
#define XFOREACH_ROOM_PROPERTY(X) \
    X(RoomName, Name, ) \
    X(RoomStaticDesc, StaticDescription, ) \
    X(RoomDynamicDesc, DynamicDescription, ) \
    X(RoomNote, Note, ) \
    X(RoomMobFlags, MobFlags, ) \
    X(RoomLoadFlags, LoadFlags, ) \
    X(RoomTerrainEnum, TerrainType, = RoomTerrainEnum::UNDEFINED) \
    X(RoomPortableEnum, PortableType, = RoomPortableEnum::UNDEFINED) \
    X(RoomLightEnum, LightType, = RoomLightEnum::UNDEFINED) \
    X(RoomAlignEnum, AlignType, = RoomAlignEnum::UNDEFINED) \
    X(RoomRidableEnum, RidableType, = RoomRidableEnum::UNDEFINED) \
    X(RoomSundeathEnum, SundeathType, = RoomSundeathEnum::UNDEFINED)

class Room;
using SharedRoom = std::shared_ptr<Room>;
using SharedConstRoom = std::shared_ptr<const Room>;
enum class RoomStatusEnum : uint8_t { Zombie, Temporary, Permanent };

class Room final : public std::enable_shared_from_this<Room>
{
private:
    struct this_is_private final
    {
        explicit this_is_private(int) {}
    };

    struct RoomFields final
    {
#define DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop _OptInit;
        XFOREACH_ROOM_PROPERTY(DECL_FIELD)
#undef DECL_FIELD
    };

private:
    /* WARNING: If you make any changes to the data members of Room, you'll have to modify clone() */
    RoomModificationTracker &m_tracker;
    Coordinate m_position;
    RoomFields m_fields;
    ExitsList m_exits;
    RoomId m_id = INVALID_ROOMID;
    RoomStatusEnum m_status = RoomStatusEnum::Zombie;
    bool m_borked = true;

private:
    // TODO: merge DirectionEnum and ExitDirEnum enums
    Exit &exit(DirectionEnum dir) { return m_exits[static_cast<ExitDirEnum>(dir)]; }
    Exit &exit(ExitDirEnum dir) { return m_exits[dir]; }

public:
    const Exit &exit(DirectionEnum dir) const { return m_exits[static_cast<ExitDirEnum>(dir)]; }
    const Exit &exit(ExitDirEnum dir) const { return m_exits[dir]; }
    const ExitsList &getExitsList() const { return m_exits; }

public:
    void setExitsList(const ExitsList &newExits);

public:
    void addInExit(ExitDirEnum dir, RoomId id);
    void addOutExit(ExitDirEnum dir, RoomId id);
    void addInOutExit(ExitDirEnum dir, RoomId id)
    {
        addInExit(dir, id);
        addOutExit(dir, id);
    }

public:
    void removeInExit(ExitDirEnum dir, RoomId id);
    void removeOutExit(ExitDirEnum dir, RoomId id);

public:
    void updateExitField(ExitDirEnum dir, const ExitFieldVariant &update);
    void modifyExitFlags(ExitDirEnum dir, FlagModifyModeEnum mode, const ExitFieldVariant &var);

public:
    ExitDirections getOutExits() const;
    OptionalExitDirConstRef getRandomExit() const;
    ExitDirConstRef getExitMaybeRandom(ExitDirEnum dir) const;

public:
    void setId(RoomId id);
    void setPosition(const Coordinate &c);
    RoomId getId() const { return m_id; }
    const Coordinate &getPosition() const { return m_position; }
    // Temporary rooms are created by the path machine during experimentation.
    // It's not clear why it can't track their "temporary" status itself.
    bool isTemporary() const { return m_status == RoomStatusEnum::Temporary; }
    void setPermanent();

    void setAboutToDie();

    // "isn't suspected of being borked?"
    bool isUpToDate() const { return !m_borked; }
    // "setNotProbablyBorked"
    void setUpToDate();
    // "setProbablyBorked"
    void setOutDated();

    void setModified(RoomUpdateFlags updateFlags);

public:
#define DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    inline const _Type &get##_Prop() const { return m_fields._Prop; } \
    void set##_Prop(_Type value);
    XFOREACH_ROOM_PROPERTY(DECL_GETTERS_AND_SETTERS)
#undef DECL_GETTERS_AND_SETTERS

public:
    Room() = delete;
    explicit Room(this_is_private, RoomModificationTracker &tracker, RoomStatusEnum status);
    ~Room();

    DELETE_CTORS_AND_ASSIGN_OPS(Room);

public:
    static void update(Room &, const ParseEvent &event);
    static void update(Room *target, const Room *source);

public:
public:
    static std::shared_ptr<Room> createPermanentRoom(RoomModificationTracker &tracker);
    static std::shared_ptr<Room> createTemporaryRoom(RoomModificationTracker &tracker,
                                                     const ParseEvent &);

public:
    static ComparisonResultEnum compare(const Room *, const ParseEvent &event, int tolerance);
    static ComparisonResultEnum compareWeakProps(const Room *, const ParseEvent &event);
    static std::shared_ptr<ParseEvent> getEvent(const Room *);

public:
    static const Coordinate &exitDir(ExitDirEnum dir);

private:
    static ComparisonResultEnum compareStrings(const std::string &room,
                                               const std::string &event,
                                               int prevTolerance,
                                               bool updated = true);

public:
    std::shared_ptr<Room> clone(RoomModificationTracker &tracker) const;
};
