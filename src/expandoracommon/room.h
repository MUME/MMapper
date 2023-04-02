#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cassert>
#include <memory>
#include <optional>
#include <QDebug>
#include <QVariant>

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"
#include "../global/TextUtils.h"
#include "../global/roomid.h"
#include "../global/roomserverid.h"
#include "../mapdata/mmapper2exit.h"
#include "../mapdata/mmapper2room.h"
#include "coordinate.h"
#include "exit.h"

class ExitFieldVariant;
class ParseEvent;

#define X_FOREACH_FlagModifyModeEnum(X) \
    X(SET) \
    X(UNSET) \
    X(TOGGLE)
#define DECL(X) X,
enum class NODISCARD FlagModifyModeEnum { X_FOREACH_FlagModifyModeEnum(DECL) };
#undef DECL
enum class NODISCARD ComparisonResultEnum { DIFFERENT = 0, EQUAL, TOLERANCE };

using ExitsList = EnumIndexedArray<Exit, ExitDirEnum, NUM_EXITS>;

struct NODISCARD ExitDirConstRef final
{
    const ExitDirEnum dir;
    const Exit &exit;
    explicit ExitDirConstRef(ExitDirEnum dir, const Exit &exit);
};

using OptionalExitDirConstRef = std::optional<ExitDirConstRef>;

class Room;

enum class NODISCARD RoomUpdateEnum {
    Id,
    ServerId,
    Coord,
    NodeLookupKey,

    Mesh,
    ConnectionsIn,
    ConnectionsOut,

    Name,
    Desc,
    Contents,
    Note,
    Terrain,

    DoorFlags,
    DoorName,
    ExitFlags,
    LoadFlags,
    MobFlags,

    Borked,
};

static constexpr const size_t NUM_ROOM_UPDATE_TYPES = 18;
static_assert(NUM_ROOM_UPDATE_TYPES == static_cast<int>(RoomUpdateEnum::Borked) + 1);
DEFINE_ENUM_COUNT(RoomUpdateEnum, NUM_ROOM_UPDATE_TYPES)

struct NODISCARD RoomUpdateFlags final : enums::Flags<RoomUpdateFlags, RoomUpdateEnum, uint32_t>
{
    using Flags::Flags;
};

class NODISCARD RoomModificationTracker
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
    NODISCARD bool isModified() const { return m_isModified; }
    void clearModified() { m_isModified = false; }

public:
    NODISCARD bool getNeedsMapUpdate() const { return m_needsMapUpdate; }
    void clearNeedsMapUpdate() { m_needsMapUpdate = false; }
};

// NOTE: Names are capitalized for use with getRoomName() and setRoomName(),
// which means they'll be capitalized as RoomFieldFlags::RoomName.
#define XFOREACH_ROOM_PROPERTY(X) \
    X(RoomName, Name, ) \
    X(RoomDesc, Description, ) \
    X(RoomContents, Contents, ) \
    X(RoomNote, Note, ) \
    X(RoomMobFlags, MobFlags, ) \
    X(RoomLoadFlags, LoadFlags, ) \
    X(RoomTerrainEnum, TerrainType, RoomTerrainEnum::UNDEFINED) \
    X(RoomPortableEnum, PortableType, RoomPortableEnum::UNDEFINED) \
    X(RoomLightEnum, LightType, RoomLightEnum::UNDEFINED) \
    X(RoomAlignEnum, AlignType, RoomAlignEnum::UNDEFINED) \
    X(RoomRidableEnum, RidableType, RoomRidableEnum::UNDEFINED) \
    X(RoomSundeathEnum, SundeathType, RoomSundeathEnum::UNDEFINED)

class Room;
using SharedRoom = std::shared_ptr<Room>;
using SharedConstRoom = std::shared_ptr<const Room>;
enum class RoomStatusEnum : uint8_t { Zombie, Temporary, Permanent };

class NODISCARD Room final : public std::enable_shared_from_this<Room>
{
private:
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };

    struct NODISCARD RoomFields final
    {
#define DECL_FIELD(_Type, _Prop, _OptInit) _Type _Prop{_OptInit};
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
    RoomServerId m_serverid = UNKNOWN_ROOMSERVERID;
    RoomStatusEnum m_status = RoomStatusEnum::Zombie;
    bool m_borked = true;

private:
    NODISCARD Exit &exit(ExitDirEnum dir)
    {
        return m_exits[dir];
    }

public:
    NODISCARD const Exit &exit(ExitDirEnum dir) const
    {
        return m_exits[dir];
    }
    NODISCARD const ExitsList &getExitsList() const
    {
        return m_exits;
    }

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
    NODISCARD ExitDirFlags getOutExits() const;
    NODISCARD OptionalExitDirConstRef getRandomExit() const;
    NODISCARD ExitDirConstRef getExitMaybeRandom(ExitDirEnum dir) const;

public:
#define DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD inline const _Type &get##_Type(ExitDirEnum dir) const \
    { \
        return exit(dir).get##_Type(); \
    } \
    void set##_Type(ExitDirEnum dir, _Type value);
    XFOREACH_EXIT_PROPERTY(DECL_GETTERS_AND_SETTERS)
#undef DECL_GETTERS_AND_SETTERS

public:
    void setId(RoomId id);
    void setServerId(RoomServerId id);
    void setPosition(const Coordinate &c);
    NODISCARD RoomId getId() const
    {
        return m_id;
    }
    NODISCARD RoomServerId getServerId() const
    {
        return m_serverid;
    }
    NODISCARD const Coordinate &getPosition() const
    {
        return m_position;
    }
    // Temporary rooms are created by the path machine during experimentation.
    // It's not clear why it can't track their "temporary" status itself.
    NODISCARD bool isTemporary() const
    {
        return m_status == RoomStatusEnum::Temporary;
    }
    void setPermanent();

    void setAboutToDie();

    // "isn't suspected of being borked?"
    NODISCARD bool isUpToDate() const
    {
        return !m_borked;
    }
    // "setNotProbablyBorked"
    void setUpToDate();
    // "setProbablyBorked"
    void setOutDated();

    void setModified(RoomUpdateFlags updateFlags);

public:
#define DECL_GETTERS_AND_SETTERS(_Type, _Prop, _OptInit) \
    NODISCARD inline const _Type &get##_Prop() const \
    { \
        return m_fields._Prop; \
    } \
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
    NODISCARD std::string toStdString() const;
    NODISCARD QString toQString() const
    {
        return ::toQStringLatin1(toStdString());
    }
    explicit operator QString() const
    {
        return toQString();
    }
    friend QDebug operator<<(QDebug os, const Room &r)
    {
        return os << r.toQString();
    }

public:
    NODISCARD static std::shared_ptr<Room> createPermanentRoom(RoomModificationTracker &tracker);
    NODISCARD static std::shared_ptr<Room> createTemporaryRoom(RoomModificationTracker &tracker,
                                                               const ParseEvent &);

public:
    NODISCARD static ComparisonResultEnum compare(const Room *,
                                                  const ParseEvent &event,
                                                  int tolerance);
    NODISCARD static ComparisonResultEnum compareWeakProps(const Room *, const ParseEvent &event);
    NODISCARD static std::shared_ptr<ParseEvent> getEvent(const Room *);

public:
    NODISCARD static const Coordinate &exitDir(ExitDirEnum dir);

private:
    NODISCARD static ComparisonResultEnum compareServerIds(const RoomServerId room,
                                                           const RoomServerId event);

    NODISCARD static ComparisonResultEnum compareStrings(const std::string &room,
                                                         const std::string &event,
                                                         int prevTolerance,
                                                         bool updated = true);

public:
    NODISCARD std::shared_ptr<Room> clone(RoomModificationTracker &tracker) const;
};
