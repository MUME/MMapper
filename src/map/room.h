#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"
#include "../global/TextUtils.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "ExitFlags.h"
#include "coordinate.h"
#include "exit.h"
#include "mmapper2room.h"
#include "roomid.h"

#include <cassert>
#include <memory>
#include <optional>
#include <sstream>

#include <QDebug>
#include <QVariant>

class ExitFieldVariant;
class ParseEvent;

enum class NODISCARD ComparisonResultEnum { DIFFERENT = 0, EQUAL, TOLERANCE };

enum class NODISCARD RoomUpdateEnum {
    BoundsChanged,
    RoomMeshNeedsUpdate,
};

static constexpr const size_t NUM_ROOM_UPDATE_TYPES = 2;
static_assert(NUM_ROOM_UPDATE_TYPES == static_cast<int>(RoomUpdateEnum::RoomMeshNeedsUpdate) + 1);
DEFINE_ENUM_COUNT(RoomUpdateEnum, NUM_ROOM_UPDATE_TYPES)

struct NODISCARD RoomUpdateFlags final
    : public enums::Flags<RoomUpdateFlags, RoomUpdateEnum, uint16_t>
{
    using Flags::Flags;
};

class NODISCARD RoomModificationTracker
{
private:
    bool m_needsMapUpdate = false;

public:
    virtual ~RoomModificationTracker();

public:
    void notifyModified(RoomUpdateFlags updateFlags);

private:
    virtual void virt_onNotifyModified(RoomUpdateFlags /*updateFlags*/) = 0;

public:
    NODISCARD bool getNeedsMapUpdate() const { return m_needsMapUpdate; }
    void clearNeedsMapUpdate() { m_needsMapUpdate = false; }
};

// X(_Type, _Name, _Init)
#define XFOREACH_ROOM_STRING_PROPERTY(X) \
    X(RoomName, Name, ) \
    X(RoomDesc, Description, ) \
    X(RoomContents, Contents, ) \
    X(RoomNote, Note, )

// X(_Type, _Name, _Init)
#define XFOREACH_ROOM_FLAG_PROPERTY(X) \
    X(RoomLoadFlags, LoadFlags, ) \
    X(RoomMobFlags, MobFlags, )

// X(_Type, _Name, _Init)
#define XFOREACH_ROOM_ENUM_PROPERTY(X) \
    X(RoomAlignEnum, AlignType, RoomAlignEnum::UNDEFINED) \
    X(RoomLightEnum, LightType, RoomLightEnum::UNDEFINED) \
    X(RoomPortableEnum, PortableType, RoomPortableEnum::UNDEFINED) \
    X(RoomRidableEnum, RidableType, RoomRidableEnum::UNDEFINED) \
    X(RoomSundeathEnum, SundeathType, RoomSundeathEnum::UNDEFINED) \
    X(RoomTerrainEnum, TerrainType, RoomTerrainEnum::UNDEFINED)

// NOTE: Names are capitalized for use with getRoomName() and setRoomName(),
// which means they'll be capitalized as RoomFields::RoomName.
// X(_Type, _Name, _Init)
#define XFOREACH_ROOM_PROPERTY(X) \
    XFOREACH_ROOM_STRING_PROPERTY(X) \
    XFOREACH_ROOM_FLAG_PROPERTY(X) \
    XFOREACH_ROOM_ENUM_PROPERTY(X)

enum class NODISCARD RoomStatusEnum : uint8_t { Temporary, Permanent, Zombie };

NODISCARD extern ComparisonResultEnum compareStrings(std::string_view room,
                                                     std::string_view event,
                                                     int prevTolerance,
                                                     bool upToDate = true);
