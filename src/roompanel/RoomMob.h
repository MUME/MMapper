#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/EnumIndexedArray.h"
#include "../global/Flags.h"
#include "../global/RuleOf5.h"

#include <memory>
#include <utility>
#include <vector>

#include <QColor>
#include <QVariant>

// -----------------------------------------------------------------------------

enum class NODISCARD MobFieldEnum : uint8_t {
    NAME = 0,
    DESC = 1,
    FIGHTING = 2,
    FLAGS = 3,
    LABELS = 4,
    MOUNT = 5,
    POSITION = 6,
    WEAPON = 7,
};

static constexpr const uint8_t NUM_MOB_FIELDS = 8u;

DEFINE_ENUM_COUNT(MobFieldEnum, NUM_MOB_FIELDS)

class NODISCARD MobFieldFlags final : public enums::Flags<MobFieldFlags, MobFieldEnum, uint8_t>
{
public:
    using Flags::Flags; // define MobFieldFlags constructors
};

// -----------------------------------------------------------------------------

class RoomMobData;
class RoomMob;
class RoomMobUpdate;
using SharedRoomMob = std::shared_ptr<RoomMob>;

// -----------------------------------------------------------------------------
// represent the data inside a RoomMob or RoomMobUpdate
class NODISCARD RoomMobData
{
public:
    using Field = MobFieldEnum;
    using Id = uint32_t; // matches QVariant::toUint() return type
    static constexpr const Id NOID = 0;

    RoomMobData();
    ~RoomMobData();
    DEFAULT_CTORS_AND_ASSIGN_OPS(RoomMobData);

public:
    NODISCARD Id getId() const { return m_id; }
    void setId(const Id id) { m_id = id; }

    NODISCARD const QVariant &getField(const Field index) const { return m_fields.at(index); }
    void setField(const Field index, QVariant value) { m_fields[index] = std::move(value); }

protected:
    using MobFieldList = EnumIndexedArray<QVariant, MobFieldEnum, NUM_MOB_FIELDS>;

    MobFieldList m_fields;
    Id m_id;
};

// -----------------------------------------------------------------------------
// represent a mob in current room, as received from GCMP messages Room.Chars.*
class NODISCARD RoomMob final : public RoomMobData, public std::enable_shared_from_this<RoomMob>
{
private:
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };

public:
    RoomMob() = delete;
    explicit RoomMob(this_is_private);
    virtual ~RoomMob();
    DELETE_CTORS_AND_ASSIGN_OPS(RoomMob);

public:
    NODISCARD static SharedRoomMob alloc();
    /// @return true if some fields changed
    NODISCARD bool updateFrom(RoomMobUpdate &&);
};

// -----------------------------------------------------------------------------
// represents a mob update, i.e. a single mob extracted from a GMCP message
// Room.Chars.Add, Room.Chars.Update or Room.Chars.Set.
//
// Differs from RoomMob because here all fields are optional:
// if a field is not present, it will not be copied into the corresponding
// field of the RoomMob with same ID.
class NODISCARD RoomMobUpdate : public RoomMobData
{
public:
    using Flags = MobFieldFlags;

    RoomMobUpdate();
    ~RoomMobUpdate();
    DEFAULT_CTORS_AND_ASSIGN_OPS(RoomMobUpdate);

public:
    NODISCARD Flags getFlags() const { return m_flags; }
    void setFlags(Flags flags) { m_flags = flags; }

    NODISCARD constexpr bool contains(const Field index) const noexcept
    {
        return m_flags.contains(index);
    }

private:
    Flags m_flags; // keeps track of which fields are present
};
