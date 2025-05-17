// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "World.h"

#include "../global/ConfigConsts.h"
#include "../global/Timer.h"
#include "../global/logging.h"
#include "../global/progresscounter.h"
#include "Diff.h"
#include "enums.h"
#include "parseevent.h"
#include "sanitizer.h"
#include "utils.h"

#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

namespace { // anonymous

// REVISIT: Should we replace this with a user-controlled option, or just make for debug builds?
// For now, it's useful to see this info in release builds;
//
// Also, we may want to try to disable this for test cases, because there are tests of invalid
// enum values, and those can trigger the error() function in the ChangePrinter.
static volatile bool print_world_changes = true;
// This limit exists because reverting may create a very large list of changes.
static volatile size_t max_change_batch_print_size = 20;

struct NODISCARD MapConsistencyError final : public std::runtime_error
{
    explicit MapConsistencyError(const std::string &msg)
        : std::runtime_error("MapConsistencyError: " + msg)
    {}
    ~MapConsistencyError() final;
};

MapConsistencyError::~MapConsistencyError() = default;

template<typename Enum>
void sanityCheckEnum(const Enum value)
{
    if (value != enums::sanitizeEnum(value)) {
        throw MapConsistencyError("invalid enum value");
    }
}
template<typename Flags>
void sanityCheckFlags(const Flags flags)
{
    if (flags != enums::sanitizeFlags(flags)) {
        throw MapConsistencyError("invalid flags");
    }
}

void insertId(RoomIdSet &set, const RoomId id)
{
    /* set = */ set.insert(id);
}

void removeId(RoomIdSet &set, const RoomId id)
{
    /* set = */ set.erase(id);
}

template<typename Key>
void insertId(OrderedMap<Key, RoomIdSet> &map, const Key &key, const RoomId id)
{
    const RoomIdSet *const old = map.find(key);
    if (old == nullptr) {
        map.set(key, RoomIdSet{id});
    } else if (!old->contains(id)) {
        auto copy = *old;
        copy.insert(id);
        map.set(key, copy);
    }
}

template<typename Key>
void removeId(OrderedMap<Key, RoomIdSet> &map, const Key &key, const RoomId id)
{
    const RoomIdSet *const old = map.find(key);
    if (old == nullptr || !old->contains(id)) {
        return;
    }

    auto copy = *old;
    copy.erase(id);
    if (copy.empty()) {
        map.erase(key);
    } else {
        map.set(key, copy);
    }
}

template<typename T>
inline void merge(T &dst, const T &src)
{
    dst = src;
}

template<>
void merge(RoomNote &dst, const RoomNote &src)
{
    dst = RoomNote{dst.toStdStringUtf8() + src.toStdStringUtf8()};
}

template<>
void merge /*<RoomMobFlags>*/ (RoomMobFlags &dst, const RoomMobFlags &src)
{
    dst |= src;
}

template<>
void merge /*<RoomLoadFlags>*/ (RoomLoadFlags &dst, const RoomLoadFlags &src)
{
    dst |= src;
}

void applyDoorName(DoorName &name, const FlagModifyModeEnum mode, const DoorName &x)
{
    switch (mode) {
    case FlagModifyModeEnum::ASSIGN:
        name = x;
        break;
    case FlagModifyModeEnum::CLEAR:
        name = {};
        break;
    case FlagModifyModeEnum::INSERT:
    case FlagModifyModeEnum::REMOVE:
        assert(false);
        break;
    }
}

template<typename Flags>
void applyFlagChange(Flags &flags, const Flags change, const FlagModifyModeEnum mode)
{
    switch (mode) {
    case FlagModifyModeEnum::ASSIGN:
        flags = enums::sanitizeFlags(change);
        break;
    case FlagModifyModeEnum::INSERT:
        flags = enums::sanitizeFlags(flags | change);
        break;
    case FlagModifyModeEnum::REMOVE:
        flags = enums::sanitizeFlags(flags & ~change);
        break;
    case FlagModifyModeEnum::CLEAR:
        flags.clear();
        break;
    }
}

void applyDoorFlags(DoorFlags &doorFlags, const FlagModifyModeEnum mode, const DoorFlags x)
{
    applyFlagChange(doorFlags, x, mode);
}

void applyExitFlags(ExitFlags &exitFlags, const FlagModifyModeEnum mode, const ExitFlags x)
{
    applyFlagChange(exitFlags, x, mode);
}

} // namespace

World World::copy() const
{
    DECL_TIMER(t, "World::copy");

    World result;
    result.m_remapping = m_remapping;
    result.m_rooms = m_rooms;
    result.m_roomSet = m_roomSet;
    result.m_spatialDb = m_spatialDb;
    result.m_serverIds = m_serverIds;
    result.m_parseTree = m_parseTree;

    return result;
}

bool World::operator==(const World &rhs) const
{
    return m_remapping == rhs.m_remapping    //
           && m_rooms == rhs.m_rooms         //
           && m_roomSet == rhs.m_roomSet     //
           && m_spatialDb == rhs.m_spatialDb //
           && m_serverIds == rhs.m_serverIds //
           && m_parseTree == rhs.m_parseTree;
}

const RawRoom *World::getRoom(const RoomId id) const
{
    if (!hasRoom(id)) {
        return nullptr;
    }

    const RawRoom &ref = m_rooms.getRawRoomRef(id);
    assert(ref.getId() == id);

    return std::addressof(ref);
}

bool World::hasRoom(const RoomId id) const
{
    if (id == INVALID_ROOMID) {
        throw InvalidMapOperation("Invalid RoomId");
    }

    if ((false)) {
        // this is O(logN)
        return m_roomSet.contains(id);
    } else {
        // this should be O(1) lookup in a vector.
        return m_remapping.contains(id);
    }
}

void World::requireValidRoom(const RoomId id) const
{
    if (!hasRoom(id)) {
        throw InvalidMapOperation("RoomId not valid");
    }
}

std::optional<RoomId> World::findRoom(const Coordinate &coord) const
{
    if (const RoomId *const id = m_spatialDb.findUnique(coord)) {
        return *id;
    }
    return std::nullopt;
}

ServerRoomId World::getServerId(const RoomId id) const
{
    requireValidRoom(id);
    return m_rooms.getServerId(id);
}

std::optional<RoomId> World::lookup(const ServerRoomId id) const
{
    return m_serverIds.lookup(id);
}

const Coordinate &World::getPosition(const RoomId id) const
{
    requireValidRoom(id);
    return m_rooms.getPosition(id);
}

#define X_DEFINE_ACCESSORS(_Type, _Name, _Init) \
    const _Type &World::getExit##_Type(const RoomId id, const ExitDirEnum dir) const \
    { \
        requireValidRoom(id); \
        return m_rooms.getExit##_Type(id, dir); \
    }
XFOREACH_EXIT_FLAG_PROPERTY(X_DEFINE_ACCESSORS)
#undef X_DEFINE_ACCESSORS

DoorName World::getExitDoorName(const RoomId id, const ExitDirEnum dir) const
{
    requireValidRoom(id);
    return m_rooms.getExitDoorName(id, dir);
}

bool World::hasExit(const RoomId id, ExitDirEnum dir) const
{
    return getExitFlags(id, dir).isExit();
}

#define X_DEFINE_GETTER(_Type, _Prop, _OptInit) \
    _Type World::get##_Type(const RoomId id, const ExitDirEnum dir) const \
    { \
        return getExit##_Type(id, dir); \
    }
XFOREACH_EXIT_PROPERTY(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

#define X_DEFINE_GETTER(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool World::exitIs##CamelCase(const RoomId id, const ExitDirEnum dir) const \
    { \
        return getExitFlags(id, dir).contains(ExitFlagEnum::UPPER_CASE); \
    }
XFOREACH_EXIT_FLAG(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

#define X_DEFINE_GETTER(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool World::doorIs##CamelCase(const RoomId id, const ExitDirEnum dir) const \
    { \
        return exitIsDoor(id, dir) && getDoorFlags(id, dir).contains(DoorFlagEnum::UPPER_CASE); \
    }
XFOREACH_DOOR_FLAG(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

const TinyRoomIdSet &World::getOutgoing(const RoomId id, const ExitDirEnum dir) const
{
    return m_rooms.getExitOutgoing(id, dir);
}

const TinyRoomIdSet &World::getIncoming(const RoomId id, const ExitDirEnum dir) const
{
    return m_rooms.getExitIncoming(id, dir);
}

void World::insertParse(const RoomId id, const ParseKeyFlags parseKeys)
{
    requireValidRoom(id);

    const RoomName &name = getRoomName(id);
    const auto &desc = m_rooms.getRoomDescription(id);
    assert(sanitizer::isSanitizedMultiline(desc.getStdStringViewUtf8()));

    if (parseKeys.contains(ParseKeyEnum::Name)) {
        insertId(m_parseTree.name_only, name, id);
    }
    if (parseKeys.contains(ParseKeyEnum::Desc)) {
        insertId(m_parseTree.desc_only, desc, id);
    }
    if (parseKeys.contains(ParseKeyEnum::Name) || parseKeys.contains(ParseKeyEnum::Desc)) {
        const NameDesc nameDesc{name, desc};
        insertId(m_parseTree.name_desc, nameDesc, id);
    }
}

void World::removeParse(const RoomId id, const ParseKeyFlags parseKeys)
{
    requireValidRoom(id);

    const RoomName &name = getRoomName(id);
    const auto &desc = m_rooms.getRoomDescription(id);
    assert(sanitizer::isSanitizedMultiline(desc.getStdStringViewUtf8()));

    if (parseKeys.contains(ParseKeyEnum::Name)) {
        removeId(m_parseTree.name_only, name, id);
    }
    if (parseKeys.contains(ParseKeyEnum::Desc)) {
        removeId(m_parseTree.desc_only, desc, id);
    }
    if (parseKeys.contains(ParseKeyEnum::Name) || parseKeys.contains(ParseKeyEnum::Desc)) {
        const NameDesc nameDesc{name, desc};
        removeId(m_parseTree.name_desc, nameDesc, id);
    }
}

ParseKeyFlags World::parseKeysChanged(const RawRoom &a, const RawRoom &b)
{
    ParseKeyFlags result;
    if (a.fields.Name != b.fields.Name) {
        result.insert(ParseKeyEnum::Name);
    }
    if (a.fields.Description != b.fields.Description) {
        result.insert(ParseKeyEnum::Desc);
    }
    return result;
}

void World::setRoom(const RoomId id, const RawRoom &room)
{
    if (id == INVALID_ROOMID) {
        throw InvalidMapOperation("Invalid RoomId");
    }
    if (room.id != id) {
        throw InvalidMapOperation("RoomId mismatch");
    }

    ParseKeyFlags parseChanged = ALL_PARSE_KEY_FLAGS;
    std::optional<Coordinate> oldCoord;
    ServerRoomId oldServerId = INVALID_SERVER_ROOMID;
    if (hasRoom(id)) {
        // REVISIT: do we bother with this?
        const auto oldRaw = getRawCopy(id);
        if (room == oldRaw) {
            return;
        }

        oldServerId = oldRaw.server_id;
        oldCoord = oldRaw.position;
        parseChanged = parseKeysChanged(oldRaw, room);
        if (parseChanged) {
            removeParse(id, parseChanged);
        }
    } else {
        // Not found
        insertId(m_roomSet, id);
    }

    if (oldServerId != INVALID_SERVER_ROOMID && oldServerId != room.server_id) {
        m_serverIds.remove(oldServerId);
    }

    if (oldCoord && oldCoord != room.position) {
        const auto &coord = room.position;
        m_spatialDb.remove(id, coord);
    }

    const auto serverId = room.server_id;
    const auto newCoord = room.position;
    {
        // REVISIT: clear first?
        setRoom_lowlevel(id, room);
    }

    if (parseChanged) {
        insertParse(id, parseChanged);
    }

    if (oldServerId != serverId) {
        m_serverIds.set(serverId, id);
    }

    if (newCoord != oldCoord) {
        const auto &coord = newCoord;
        m_spatialDb.add(id, coord);
    }

    if constexpr (IS_DEBUG_BUILD) {
        const auto &here = deref(getRoom(id));
        assert(satisfiesInvariants(here));

        auto copy = room;
        enforceInvariants(copy);
        assert(here == copy);
    }
}

bool World::hasOneWayExit_inconsistent(const RoomId from,
                                       const ExitDirEnum dir,
                                       const InOutEnum mode,
                                       const RoomId to) const
{
    if (hasRoom(from)) {
        return m_rooms.getExitInOut(from, dir, mode).contains(to);
    }
    return false;
}

bool World::hasTwoWayExit_inconsistent(const RoomId from,
                                       const ExitDirEnum dir,
                                       const InOutEnum mode,
                                       const RoomId to) const
{
    return hasOneWayExit_inconsistent(from, dir, mode, to)
           && hasOneWayExit_inconsistent(to, opposite(dir), mode, from);
}

bool World::hasConsistentOneWayExit(const RoomId from, const ExitDirEnum dir, const RoomId to) const
{
    return hasOneWayExit_inconsistent(from, dir, InOutEnum::Out, to)
           && hasOneWayExit_inconsistent(to, opposite(dir), InOutEnum::In, from);
}

bool World::hasConsistentTwoWayExit(const RoomId from, const ExitDirEnum dir, const RoomId to) const
{
    return hasTwoWayExit_inconsistent(from, dir, InOutEnum::Out, to)
           && hasTwoWayExit_inconsistent(to, opposite(dir), InOutEnum::Out, from);
}

bool World::hasConsistentExit(const RoomId from,
                              const ExitDirEnum dir,
                              const RoomId to,
                              const WaysEnum ways) const
{
    switch (ways) {
    case WaysEnum::OneWay:
        return hasConsistentOneWayExit(from, dir, to);
    case WaysEnum::TwoWay:
        return hasConsistentTwoWayExit(from, dir, to);
    }
    std::abort();
}

void World::addExit_inconsistent(const RoomId from,
                                 const ExitDirEnum dir,
                                 const InOutEnum mode,
                                 const RoomId to)
{
    if (!hasRoom(from)) {
        throw InvalidMapOperation("RoomId not found");
    }

    const TinyRoomIdSet &view = m_rooms.getExitInOut(from, dir, mode);
    if (!view.contains(to)) {
        RoomIdSet tmp = toRoomIdSet(view);
        tmp.insert(to);
        m_rooms.setExitInOut(from, dir, mode, toTinyRoomIdSet(tmp));
    }

    assert(hasOneWayExit_inconsistent(from, dir, mode, to));
}

void World::addConsistentOneWayExit(const RoomId from, const ExitDirEnum dir, const RoomId to)
{
    addExit_inconsistent(from, dir, InOutEnum::Out, to);
    addExit_inconsistent(to, opposite(dir), InOutEnum::In, from);
    assert(hasConsistentOneWayExit(from, dir, to));
}

void World::addExit(const RoomId from, const ExitDirEnum dir, const RoomId to, const WaysEnum ways)
{
    if (hasConsistentExit(from, dir, to, ways)) {
        return;
    }

    addConsistentOneWayExit(from, dir, to);

    switch (ways) {
    case WaysEnum::OneWay:
        break;
    case WaysEnum::TwoWay:
        // note: recursion is limited by ways.
        addExit(to, opposite(dir), from, WaysEnum::OneWay);
        break;
    }

    assert(hasConsistentExit(from, dir, to, ways));
}

void World::removeExit_inconsistent(const RoomId from,
                                    const ExitDirEnum dir,
                                    const InOutEnum mode,
                                    const RoomId to)
{
    if (hasRoom(from)) {
        const TinyRoomIdSet &view = m_rooms.getExitInOut(from, dir, mode);
        if (view.contains(to)) {
            if (view.size() == 1) {
                m_rooms.setExitInOut(from, dir, mode, TinyRoomIdSet{});
            } else if (view.size() == 2) {
                RoomId tmp = INVALID_ROOMID;
                for (const RoomId x : view) {
                    if (x != to) {
                        tmp = x;
                    }
                }
                assert(tmp != INVALID_ROOMID);
                m_rooms.setExitInOut(from, dir, mode, TinyRoomIdSet{tmp});
            } else {
                TinyRoomIdSet copy = view;
                copy.erase(to);
                m_rooms.setExitInOut(from, dir, mode, copy);
            }
        }
    }
    assert(!hasOneWayExit_inconsistent(from, dir, mode, to));
}

void World::removeExit_consistently(const RoomId from, const ExitDirEnum dir, const RoomId to)
{
    removeExit_inconsistent(from, dir, InOutEnum::Out, to);
    removeExit_inconsistent(to, opposite(dir), InOutEnum::In, from);
}

void World::removeExit(const RoomId from,
                       const ExitDirEnum dir,
                       const RoomId to,
                       const WaysEnum ways)
{
    removeExit_consistently(from, dir, to);

    switch (ways) {
    case WaysEnum::OneWay:
        break;
    case WaysEnum::TwoWay:
        // note: recursion is limited by ways.
        removeExit(to, opposite(dir), from, WaysEnum::OneWay);
        break;
    }
}

void World::checkAllExitsConsistent(RoomId id) const
{
    if (!hasRoom(id)) {
        throw InvalidMapOperation("RoomId not found");
    }

    for (const ExitDirEnum dir : ALL_EXITS7) {
        const auto rev = opposite(dir);
        const auto &out = m_rooms.getExitOutgoing(id, dir);
        for (const RoomId other : out) {
            if (!hasOneWayExit_inconsistent(other, rev, InOutEnum::In, id)) {
                throw MapConsistencyError("missing incoming one-way exit");
            }
        }
        const auto &in = m_rooms.getExitIncoming(id, dir);
        for (const RoomId other : in) {
            if (!hasOneWayExit_inconsistent(other, rev, InOutEnum::Out, id)) {
                throw MapConsistencyError("missing outgoing one-way exit");
            }
        }
    }
}

void World::checkConsistency(ProgressCounter &counter) const
{
    if (getRoomSet().empty()) {
        return;
    }

    DECL_TIMER(t, __FUNCTION__);

    auto checkPosition = [this](const RoomId id) {
        const Coordinate &coord = getPosition(id);
        // Is there a unique owner of the coord?
        if (const RoomId *const maybe = m_spatialDb.findUnique(coord);
            maybe == nullptr || *maybe != id) {
            throw MapConsistencyError("two rooms using the same coordinate found");
        }
    };

    auto checkServerId = [this](const RoomId id) {
        const ServerRoomId serverId = getServerId(id);
        if (serverId != INVALID_SERVER_ROOMID && !m_serverIds.contains(serverId)) {
            // throw MapConsistencyError("...")
            qWarning() << "Room" << id.asUint32() << "server id" << serverId.asUint32()
                       << "does not map to a room.";
        }
    };

    auto checkExitFlags = [this](const RoomId id, const ExitDirEnum dir) {
        const auto exitFlags = getExitFlags(id, dir);
        const auto doorFlags = getDoorFlags(id, dir);

        sanityCheckFlags(doorFlags);
        sanityCheckFlags(exitFlags);

        const bool hasDoorName = !m_rooms.getExitDoorName(id, dir).empty();
        if (hasDoorName) {
            if (!sanitizer::isSanitizedOneLine(
                    m_rooms.getExitDoorName(id, dir).getStdStringViewUtf8())) {
                throw MapConsistencyError("door name fails sanity check");
            }
        }

        if (!satisfiesInvariants(m_rooms.getRawRoomRef(id).getExit(dir))) {
            throw MapConsistencyError("room exit flags do not satisfy invariants");
        }
    };

    auto checkFlags = [this, &checkExitFlags](const RoomId id) {
        sanityCheckFlags(m_rooms.getRoomLoadFlags(id));
        sanityCheckFlags(m_rooms.getRoomMobFlags(id));
        for (const ExitDirEnum dir : ALL_EXITS7) {
            checkExitFlags(id, dir);
        }
    };

    auto checkRemapping = [this](const RoomId id) {
        if (!m_roomSet.contains(id)) {
            throw MapConsistencyError("remapping cache already contained this id");
        }

        if (!m_remapping.contains(id)) {
            throw MapConsistencyError("remapping did not contain this id");
        }

        const ExternalRoomId ext = this->convertToExternal(id);
        if (convertToInternal(ext) != id) {
            throw MapConsistencyError("unable to convert to internal id");
        }
    };

    auto checkParseTree = [this](const RoomId id) {
        const RoomName &name = getRoomName(id);
        const RoomDesc &desc = m_rooms.getRoomDescription(id);

        if (auto set = m_parseTree.name_only.find(name); set == nullptr || !set->contains(id)) {
            throw MapConsistencyError("unable to find room name only");
        }

        if (auto set = m_parseTree.desc_only.find(desc); set == nullptr || !set->contains(id)) {
            throw MapConsistencyError("unable to find room desc only");
        }

        {
            const NameDesc nameDesc{name, desc};
            if (auto set = m_parseTree.name_desc.find(nameDesc);
                set == nullptr || !set->contains(id)) {
                throw MapConsistencyError("unable to find room name_desc only");
            }
        }
    };

    auto checkEnums = [this](const RoomId id) {
        sanityCheckEnum(m_rooms.getRoomAlignType(id));
        sanityCheckEnum(m_rooms.getRoomLightType(id));
        sanityCheckEnum(m_rooms.getRoomPortableType(id));
        sanityCheckEnum(m_rooms.getRoomRidableType(id));
        sanityCheckEnum(m_rooms.getRoomSundeathType(id));
        sanityCheckEnum(m_rooms.getRoomTerrainType(id));
    };

    counter.setNewTask(ProgressMsg{"checking room consistency"}, getRoomSet().size());
    for (const RoomId id : getRoomSet()) {
        checkAllExitsConsistent(id);
        checkEnums(id);
        checkFlags(id);
        checkParseTree(id);
        checkPosition(id);
        checkRemapping(id);
        checkServerId(id);
        counter.step();
    }

    {
        counter.setNewTask(ProgressMsg{"checking server ids"}, m_serverIds.size());
        m_serverIds.for_each([this, &counter](const ServerRoomId serverId, const RoomId id) {
            if (this->getServerId(id) != serverId) {
                throw MapConsistencyError("room server id was not the expected value");
            }
            counter.step();
        });
    }

    {
        if (m_spatialDb.needsBoundsUpdate()) {
            throw MapConsistencyError("needs bounds update");
        }

        counter.setNewTask(ProgressMsg{"checking map coordinates"}, m_spatialDb.size());
        m_spatialDb.for_each([this, &counter](const Coordinate &coord, const RoomId id) {
            if (this->getPosition(id) != coord) {
                throw MapConsistencyError("room position was not the expected coord");
            }
            counter.step();
        });

        const auto &knownBounds = deref(m_spatialDb.getBounds());

        // Doing it this way is like asking the fox to guard the hen house,
        // but above we've verified that all of the coordinates are in the db,
        {
            auto spatialDb_copy = m_spatialDb;
            counter.setNewTask(ProgressMsg{"recomputing bounds"}, 1);
            spatialDb_copy.updateBounds(counter);
            counter.step();
            const auto &computedBounds = deref(spatialDb_copy.getBounds());
            if (knownBounds != computedBounds) {
                throw MapConsistencyError("known bounds were not the computed bounds");
            }
        }

        // This is better.
        if (!getRoomSet().empty()) {
            std::optional<Bounds> computedBounds;
            counter.setNewTask(ProgressMsg{"checking map coordinates"}, getRoomSet().size());
            for (const RoomId id : getRoomSet()) {
                const Coordinate &coord = getPosition(id);
                if (!computedBounds) {
                    computedBounds.emplace(coord, coord);
                } else {
                    computedBounds->insert(coord);
                }
                counter.step();
            }
            if (computedBounds != knownBounds) {
                // REVISIT: This is happening for the "fullarda.mm2" map
                throw MapConsistencyError("computed bounds were not the known bounds");
            }
        }
    }

    // REVISIT: Check max id?
}

void World::nukeHelper(const RoomId id,
                       const ExitDirEnum dir,
                       const RawExit &ex,
                       const WaysEnum ways)
{
    for (const RoomId other : ex.outgoing) {
        removeExit(id, dir, other, ways);
    }

    if (ways == WaysEnum::TwoWay) {
        const auto rev = opposite(dir);
        for (const RoomId other : ex.incoming) {
            removeExit(other, rev, id, ways);
        }
    }
}

void World::clearExit(const RoomId id, const ExitDirEnum dir, const WaysEnum ways)
{
    auto &exitRef = m_rooms.getRawRoomRef(id).getExit(dir);
    if (ways == WaysEnum::OneWay) {
        // copy could allocate (about 0.1% of outgoing and 0.3% of incoming),
        // so we'll only do it for the one-way case.
        TinyRoomIdSet old_inbound = std::exchange(exitRef.incoming, {});
        exitRef = {};
        exitRef.incoming = std::move(old_inbound);
    } else {
        exitRef = {};
    }
}

void World::nukeExit(const RoomId id, const ExitDirEnum dir, const WaysEnum ways)
{
    if (!hasRoom(id)) {
        return;
    }

    const RawExit copiedExit = getRawExit(id, dir);
    clearExit(id, dir, ways);
    nukeHelper(id, dir, copiedExit, ways);
}

void World::nukeAllExits(const RoomId id, const WaysEnum ways)
{
    if (!hasRoom(id)) {
        return;
    }

    using Exits = EnumIndexedArray<RawExit, ExitDirEnum, NUM_EXITS>;
    Exits copiedExits;
    for (const ExitDirEnum dir : ALL_EXITS7) {
        copiedExits[dir] = getRawExit(id, dir);
        clearExit(id, dir, ways);
    }

    for (const ExitDirEnum dir : ALL_EXITS7) {
        const RawExit &ex = copiedExits[dir];
        nukeHelper(id, dir, ex, ways);
    }
}

void World::setServerId(const RoomId id, const ServerRoomId serverId)
{
    requireValidRoom(id);

    const auto oldServerId = getServerId(id);
    if (oldServerId == serverId) {
        return;
    }

    m_serverIds.remove(oldServerId);
    m_rooms.setServerId(id, serverId);
    m_serverIds.set(serverId, id);
}

void World::setPosition(const RoomId id, const Coordinate &coord)
{
    requireValidRoom(id);

    if (getPosition(id) == coord) {
        return;
    }

    const Coordinate &ref = m_rooms.getPosition(id);
    m_spatialDb.move(id, ref, coord);
    m_rooms.setPosition(id, coord);
}

bool World::wouldAllowRelativeMove(const RoomIdSet &rooms, const Coordinate &offset) const
{
    if (rooms.empty()) {
        return false;
    }
    for (const auto id : rooms) {
        if (!hasRoom(id)) {
            return false; // avoid throwing
        }
        const auto &here = getPosition(id); // throws if missing
        const auto there = here + offset;
        if (auto other = findRoom(there)) {
            if (!rooms.contains(*other)) {
                return false;
            }
        }
    }
    return true;
}

void World::moveRelative(const RoomId id, const Coordinate &offset)
{
    setPosition(id, getPosition(id) + offset);
}

void World::moveRelative(const RoomIdSet &rooms, const Coordinate &offset)
{
    if (rooms.empty()) {
        throw std::runtime_error("no rooms specified");
    }

    if (!wouldAllowRelativeMove(rooms, offset)) {
        throw std::runtime_error("invalid batch movement");
    }

    volatile bool use_hack = false;
    if (use_hack) {
        // This isn't advised, since it doesn't handle potential signed integer under/overflow.
        int zimpossible = 1;
        if (auto bounds = getBounds()) {
            zimpossible = std::max(0, bounds->max.z - bounds->min.z) + 1;
        }
        qInfo() << "zimpossible = " << zimpossible;
        for (const auto id : rooms) {
            moveRelative(id, Coordinate{0, 0, -zimpossible});
        }
        for (const auto id : rooms) {
            moveRelative(id, offset + Coordinate{0, 0, zimpossible});
        }
    } else {
        struct NODISCARD MoveInfo final
        {
            RoomId id;
            Coordinate newPos;
        };
        std::vector<MoveInfo> infos;
        infos.reserve(rooms.size());
        for (const auto id : rooms) {
            const auto &oldPos = getPosition(id);
            infos.emplace_back(MoveInfo{id, oldPos + offset});
            m_spatialDb.remove(id, oldPos);
        }
        for (const auto &x : infos) {
            m_spatialDb.add(x.id, x.newPos);
            m_rooms.setPosition(x.id, x.newPos);
        }
    }
}

void World::updateRoom(const RawRoom &newRoom)
{
    const RoomId id = newRoom.id;
    requireValidRoom(id);

    // The only things that are allowed to be "updated" are:
    // fields
    // status
    // borked
    // exit.fields.ExitFlags
    // exit.fields.DoorFlags

    RawRoom check = getRawCopy(id);
    check.fields = newRoom.fields;
    check.status = newRoom.status;
    check.server_id = newRoom.server_id;
    for (const ExitDirEnum dir : ALL_EXITS7) {
        const auto &changed = newRoom.exits[dir].fields;
        RawExit &copy = check.exits[dir];
        copy.fields.exitFlags = changed.exitFlags;
        copy.fields.doorFlags = changed.doorFlags;
        copy.fields.doorName = changed.doorName;
    }
    if (check != newRoom) {
        throw InvalidMapOperation("Room mismatch");
    }

    setRoom(id, newRoom);
}

void World::removeFromWorld(const RoomId id, const bool removeLinks)
{
    if (id == INVALID_ROOMID) {
        throw InvalidMapOperation("Invalid RoomId");
    }

    const auto coord = getPosition(id);
    const auto server_id = getServerId(id);

    removeParse(id, ALL_PARSE_KEY_FLAGS);
    m_spatialDb.remove(id, coord);
    m_serverIds.remove(server_id);

    if (removeLinks) {
        nukeAllExits(id, WaysEnum::TwoWay);
    }

    m_remapping.removeAt(id);
    m_rooms.removeAt(id);
    removeId(m_roomSet, id);
}

void World::setRoomStatus(const RoomId id, const RoomStatusEnum status)
{
    requireValidRoom(id);
    m_rooms.setStatus(id, status);
}

void World::setRoomExitFields(const RoomId id, const ExitDirEnum dir, const ExitFields &fields)
{
    m_rooms.setExitDoorFlags(id, dir, fields.doorFlags);
    m_rooms.setExitExitFlags(id, dir, fields.exitFlags);
    m_rooms.setExitDoorName(id, dir, fields.doorName);
    m_rooms.enforceInvariants(id, dir);
}

RawExit World::getRawExit(const RoomId id, const ExitDirEnum dir) const
{
    RawExit to;
    to.fields.doorFlags = m_rooms.getExitDoorFlags(id, dir);
    to.fields.exitFlags = m_rooms.getExitExitFlags(id, dir);
    to.fields.doorName = m_rooms.getExitDoorName(id, dir);
    to.outgoing = m_rooms.getExitOutgoing(id, dir);
    to.incoming = m_rooms.getExitIncoming(id, dir);
    return to;
}

RawRoom World::getRawCopy(const RoomId id) const
{
    requireValidRoom(id);

    RawRoom result;

#define X_COPY_FIELD(_Type, _Prop, _OptInit) result.fields._Prop = m_rooms.getRoom##_Prop(id);
    XFOREACH_ROOM_PROPERTY(X_COPY_FIELD)
#undef X_COPY_FIELD

    for (const ExitDirEnum dir : ALL_EXITS7) {
        result.exits[dir] = getRawExit(id, dir);
    }

    result.position = m_rooms.getPosition(id);
    result.server_id = m_rooms.getServerId(id);
    result.id = id;
    result.status = m_rooms.getStatus(id);
    return result;
}

void World::copyStatusAndExitFields(const RawRoom &from)
{
    const RoomId id = from.id;
    setRoomStatus(id, from.status);
    for (const ExitDirEnum dir : ALL_EXITS7) {
        setRoomExitFields(id, dir, from.exits[dir].fields);
    }
}

void World::merge_update(RawRoom &target, const RawRoom &source)
{
#define X_COPY_FIELD(_Type, _Prop, _OptInit) \
    do { \
        using Type = _Type; \
        if (source.fields._Prop != Type{_OptInit}) { \
            merge((target.fields._Prop), (source.fields._Prop)); \
        } \
    } while (false);
    XFOREACH_ROOM_PROPERTY(X_COPY_FIELD)
#undef X_COPY_FIELD

    // Combine data if target room is up to date
    // REVISIT: what about UNKNOWN?
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        if (source.hasTrivialExit(dir)) {
            continue;
        }

        const RawExit &sourceExit = source.getExit(dir);
        RawExit targetExit = target.getExit(dir);

        // REVISIT: This could be done with an xmacro.
        merge(targetExit.fields.exitFlags, sourceExit.fields.exitFlags);
        merge(targetExit.fields.doorName, sourceExit.fields.doorName);
        merge(targetExit.fields.doorFlags, sourceExit.fields.doorFlags);

        // store the result
        target.exits[dir] = targetExit;
    }
}

void World::copy_exits(const RoomId targetId, const RawRoom &source)
{
    if (targetId == source.id) {
        throw InvalidMapOperation("RoomId can not match");
    }

    auto remap = [targetId, &source](const RoomId id) -> RoomId {
        return (id == source.id) ? targetId : id;
    };

    auto remapExit = [this, &remap](const RoomId from, const ExitDirEnum dir, const RoomId to) {
        // NOTE: Any existing exits between source and target will become loops!
        addExit(remap(from), dir, remap(to), WaysEnum::OneWay);
    };

    for (const ExitDirEnum dir : ALL_EXITS7) {
        const RawExit &e = source.exits[dir];
        for (const RoomId from : e.getInOut(InOutEnum::In)) {
            remapExit(from, opposite(dir), source.id);
        }
        for (const RoomId to : e.getInOut(InOutEnum::Out)) {
            remapExit(source.id, dir, to);
        }

        // If we added an exit, we need to make sure the flag exists;
        // REVISIT: should addExit() itself update the EXIT flag?
        m_rooms.enforceInvariants(source.id, dir);
    }
}

void World::mergeRelative(const RoomId id, const Coordinate &offset)
{
    if (offset.isNull()) {
        return;
    }

    const auto targetId = [this, id, &offset]() -> std::optional<RoomId> {
        const auto pos = m_rooms.getPosition(id) + offset;

        const auto optTarget = findRoom(pos);
        if (!optTarget) {
            // nothing was already there!
            setPosition(id, pos);
            return std::nullopt;
        }

        const RoomId result_targetId = *optTarget;
        if (result_targetId == id) {
            // implies offset is 0,0,0
            throw InvalidMapOperation();
        }

        {
            RawRoom target = getRawCopy(result_targetId);
            merge_update(target, getRawCopy(id));
            setRoom(target.id, target);
        }

        return result_targetId;
    }();

    if (!targetId) {
        return;
    }

    copy_exits(*targetId, getRawCopy(id));
    removeFromWorld(id, true);
}

void World::setRemapAndAllocateRooms(Remapping new_remap)
{
    assert(m_remapping.empty());
    std::swap(this->m_remapping, new_remap);

    m_rooms.resize(m_remapping.size());
}

void World::setExit(const RoomId id, const ExitDirEnum dir, const RawExit &input)
{
    assert(hasRoom(id));

    m_rooms.setExitDoorFlags(id, dir, input.fields.doorFlags);
    m_rooms.setExitExitFlags(id, dir, input.fields.exitFlags);
    m_rooms.setExitDoorName(id, dir, input.fields.doorName);
    m_rooms.setExitOutgoing(id, dir, input.outgoing);
    m_rooms.setExitIncoming(id, dir, input.incoming);
    m_rooms.enforceInvariants(id, dir);
}

void World::setRoom_lowlevel(const RoomId id, const RawRoom &input)
{
    assert(id == input.id);
    m_rooms.getRawRoomRef(id) = input;
    m_rooms.enforceInvariants(id);
}

// for addRoom()
void World::initRoom(const RawRoom &input)
{
    const RoomId id = input.id;
    assert(id != INVALID_ROOMID);
    m_rooms.requireUninitialized(id);

    /* copy the room data */
    setRoom_lowlevel(id, input);

    /* now perform bookkeeping */
    {
        // REVISIT: should "upToDate" be automatic?
        insertId(m_roomSet, id);
        insertParse(id, ALL_PARSE_KEY_FLAGS);
        m_spatialDb.add(id, input.position);
        m_serverIds.set(input.server_id, id);
    }

    if constexpr (IS_DEBUG_BUILD) {
        const auto &here = deref(getRoom(id));
        assert(satisfiesInvariants(here));

        auto copy = input;
        enforceInvariants(copy);
        assert(here == copy);
    }
}

World World::init(ProgressCounter &counter, const std::vector<ExternalRawRoom> &ext_rooms)
{
    DECL_TIMER(t, __FUNCTION__);

    World w;

    std::vector<RawRoom> rooms;
    {
        counter.setNewTask(ProgressMsg{"computing remapping"}, 3);
        Remapping remapping = Remapping::computeFrom(ext_rooms);
        counter.step();
        // REVISIT: defer the remapping to initRoom, or do it here?
        rooms = remapping.convertToInternal(ext_rooms);
        counter.step();
        assert(rooms.size() == ext_rooms.size());
        {
            DECL_TIMER(t2, "setRemapAndAllocateRooms");
            w.setRemapAndAllocateRooms(std::move(remapping));
        }
        counter.step();
    }

    {
        DECL_TIMER(t1, "insert-rooms");
        {
            DECL_TIMER(t2, "insert-rooms-part1");

            const size_t numRooms = rooms.size();

            // REVISIT: This trick is really only valid if there are no gaps in the roomids,
            // and they're all presented in order!
            {
                RoomId next{0};
                for (const auto &r : rooms) {
                    if (r.id != next++) {
                        throw std::runtime_error("elements aren't in order");
                    }
                }
                assert(next.value() == numRooms);
            }

            {
                DECL_TIMER(t3, "copy rooms");
                for (const auto &r : rooms) {
                    const RoomId id = r.getId();
                    auto &roomRef = w.m_rooms.getRawRoomRef(id);
                    roomRef = r; // copy
                }
            }
        }

        {
            DECL_TIMER(t3, "update-exit-flags");
            counter.setNewTask(ProgressMsg{"updating exit flags"}, rooms.size());
            for (const auto &room : rooms) {
                for (const ExitDirEnum dir : ALL_EXITS7) {
                    w.m_rooms.enforceInvariants(room.id, dir);
                }
                counter.step();
            }
        }

        {
            DECL_TIMER(t3, "insert-rooms-cachedRoomSet");
            counter.setNewTask(ProgressMsg{"inserting rooms"}, rooms.size());
            for (const auto &room : rooms) {
                insertId(w.m_roomSet, room.id);
                counter.step();
            }
        }
        {
            // REVISIT: slow
            DECL_TIMER(t3, "insert-rooms-parsekey");
            counter.setNewTask(ProgressMsg{"inserting room name/desc lookups"}, rooms.size());
            for (const auto &room : rooms) {
                w.insertParse(room.id, ALL_PARSE_KEY_FLAGS);
                counter.step();
            }
        }
        {
            DECL_TIMER(t3, "insert-rooms-spatialDb");
            counter.setNewTask(ProgressMsg{"setting room positions"}, rooms.size());
            for (const auto &room : rooms) {
                w.m_spatialDb.add(room.id, room.position);
                counter.step();
            }
        }
        {
            DECL_TIMER(t3, "insert-rooms-serverIds");
            counter.setNewTask(ProgressMsg{"setting room server ids"}, rooms.size());
            for (const auto &room : rooms) {
                w.m_serverIds.set(room.server_id, room.id);
                counter.step();
            }
        }
    }
    {
        DECL_TIMER(t4, "update-bounds");
        counter.setNewTask(ProgressMsg{"updating bounds"}, 1);
        w.m_spatialDb.updateBounds(counter);
        counter.step();
    }

    // if constexpr ((IS_DEBUG_BUILD))
    {
        DECL_TIMER(t5, "check-consistency");
        counter.setNewTask(ProgressMsg{"checking map consistency" /*" [debug]"*/}, 1);
        w.checkConsistency(counter);
        counter.step();
    }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    return w;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

RoomId World::getNextId() const
{
    const auto &set = getRoomSet();
    if (set.empty()) {
        return RoomId{0};
    }
    return set.last().next();
}

ExternalRoomId World::getNextExternalId() const
{
    return m_remapping.getNextExternal();
}

RoomId World::addRoom(const Coordinate &position)
{
    if (findRoom(position)) {
        throw InvalidMapOperation("Position in use");
    }

    const RoomId id = getNextId();
    if (id == INVALID_ROOMID) {
        throw InvalidMapOperation("No RoomIds available");
    }

    // Resize all vectors
    {
        const uint32_t newSize = id.asUint32() + 1;
        m_rooms.resize(newSize);
    }

    m_remapping.addNew(id);

    RawRoom r;
    r.id = id;
    r.position = position;
    r.server_id = INVALID_SERVER_ROOMID;

    initRoom(r);
    assert(hasRoom(id));
    assert(findRoom(position) == id);
    const auto ext = convertToExternal(id);
    MMLOG() << "Added new room " << ext.value() << ".";
    return id;
}

void World::undeleteRoom(const ExternalRoomId extid, const RawRoom &raw)
{
    if (extid == INVALID_EXTERNAL_ROOMID || raw.id == INVALID_ROOMID) {
        throw InvalidMapOperation("Invalid room id");
    }

    if (findRoom(raw.position)) {
        throw InvalidMapOperation("Position in use");
    }

    if (getRoom(raw.id)) {
        throw InvalidMapOperation("World already contains that room id");
    }
    if (m_remapping.convertToInternal(extid) != INVALID_ROOMID) {
        throw InvalidMapOperation("World already contains that external room id");
    }

    if (raw.id > getNextId()) {
        throw InvalidMapOperation("Cannot allocate that room id.");
    }
    if (extid > getNextExternalId()) {
        throw InvalidMapOperation("Cannoat allocate that external id.");
    }

    // Resize all vectors
    {
        const uint32_t newSize = raw.id.asUint32() + 1;
        if (newSize > m_rooms.size()) {
            m_rooms.resize(newSize);
        }
    }

    m_remapping.undelete(raw.id, extid);

    for (const auto &e : raw.exits) {
        if (!e.getOutgoingSet().empty() || !e.getIncomingSet().empty()) {
            throw std::runtime_error("exits must be restored separately");
        }
    }

    initRoom(raw);

    assert(hasRoom(raw.id));
    assert(findRoom(raw.position) == raw.id);
    const auto ext = convertToExternal(raw.id);
    if (ext != extid) {
        throw std::runtime_error("failed sanity check");
    }
    MMLOG() << "Added new room " << ext.value() << ".";
}

void World::addRoom2(const Coordinate &desiredPosition, const ParseEvent &event)
{
    const auto position = [this, desiredPosition]() -> Coordinate {
        return ::getNearestFree(desiredPosition, [this](const Coordinate &check) -> FindCoordEnum {
            return findRoom(check).has_value() ? FindCoordEnum::InUse : FindCoordEnum::Available;
        });
    }();

    const RoomId roomId = addRoom(position);

    MMLOG() << "Applying changes after adding room " << convertToExternal(roomId).value() << "...";
    ProgressCounter dummyPc;
    apply(dummyPc, room_change_types::Update{roomId, event});

    // REVISIT: Merge the duplicate code with that in pathmachine
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        const auto toServerId = event.getExitIds()[dir];
        if (toServerId == INVALID_SERVER_ROOMID) {
            continue;
        }
        const auto copiedExit = getRawExit(roomId, dir);
        if (copiedExit.outIsUnique()) {
            // Add likely ServerId
            const auto to = copiedExit.outFirst();
            if (getServerId(to) == INVALID_SERVER_ROOMID) {
                apply(dummyPc, room_change_types::SetServerId{to, toServerId});
                continue;
            }
        }
        // Add missing connections to that ServerId
        if (const auto &there = lookup(toServerId)) {
            const auto to = there.value();
            if (!copiedExit.containsOut(to)) {
                apply(dummyPc,
                      exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                              roomId,
                                                              dir,
                                                              to,
                                                              WaysEnum::OneWay});
            }
        }
    }
}

ExternalRoomIdSet World::convertToExternal(ProgressCounter &pc, const TinyRoomIdSet &set) const
{
    pc.increaseTotalStepsBy(set.size());
    ExternalRoomIdSet result;
    for (const RoomId id : set) {
        result.insert(convertToExternal(id));
        pc.step();
    }
    return result;
}

ExternalRawExit World::convertToExternal(const RawExit &exit) const
{
    return m_remapping.convertToExternal(exit);
}

ExternalRawRoom World::convertToExternal(const RawRoom &room) const
{
    return m_remapping.convertToExternal(room);
}

RoomId World::convertToInternal(const ExternalRoomId ext) const
{
    return m_remapping.convertToInternal(ext);
}

ExternalRoomId World::convertToExternal(const RoomId id) const
{
    return m_remapping.convertToExternal(id);
}

void World::apply(ProgressCounter &pc, const world_change_types::CompactRoomIds &change)
{
    m_remapping.compact(pc, change.firstId);
}

void World::apply(ProgressCounter &pc, const world_change_types::RemoveAllDoorNames & /* unused */)
{
    pc.increaseTotalStepsBy(getRoomSet().size());
    size_t numRemoved = 0;
    const DoorName none;
    for (const RoomId id : getRoomSet()) {
        for (const ExitDirEnum dir : ALL_EXITS7) {
            const ExitFlags exitFlags = m_rooms.getExitExitFlags(id, dir);
            if (!exitFlags.isExit() || !exitFlags.isDoor()
                || !m_rooms.getExitDoorFlags(id, dir).isHidden()) {
                continue;
            }

            const auto &doorName = m_rooms.getExitDoorName(id, dir);
            if (doorName.empty()) {
                continue;
            }

            m_rooms.setExitDoorName(id, dir, none);
            numRemoved += 1;
        }
        pc.step();
    }

    MMLOG() << "#NOTE: removed " << numRemoved << " hidden door name"
            << ((numRemoved == 1) ? "" : "s") << ".";
}

void World::apply(ProgressCounter &, const exit_change_types::NukeExit &change)
{
    nukeExit(change.room, change.dir, change.ways);
}

void World::apply(ProgressCounter &pc, const exit_change_types::ModifyExitConnection &change)
{
    switch (change.type) {
    case ChangeTypeEnum::Add:
        addExit(change.room, change.dir, change.to, change.ways);
        break;
    case ChangeTypeEnum::Remove:
        removeExit(change.room, change.dir, change.to, change.ways);
        break;
    }

    if (change.ways == WaysEnum::TwoWay) {
        auto copy = change;
        std::swap(copy.room, copy.to);
        copy.dir = opposite(change.dir);
        copy.ways = WaysEnum::OneWay;
        assert(copy.type == change.type);
        apply(pc, copy);
    }
}

void World::apply(ProgressCounter &, const exit_change_types::SetExitFlags &change)
{
    // REVISIT: change SetXXXFlags to include SET, OR, NAND?
    apply_update(change.room, [&change](RawRoom &r) -> void {
        RawExit &e = r.exits[change.dir];

        switch (change.type) {
        case FlagChangeEnum::Set:
            e.fields.exitFlags = change.flags;
            break;
        case FlagChangeEnum::Add:
            e.fields.exitFlags |= change.flags;
            break;
        case FlagChangeEnum::Remove:
            e.fields.exitFlags &= ~change.flags;
            break;
        }
        enforceInvariants(e);
    });
}

void World::apply(ProgressCounter &, const exit_change_types::SetDoorFlags &change)
{
    apply_update(change.room, [&change](RawRoom &r) -> void {
        RawExit &e = r.exits[change.dir];
        switch (change.type) {
        case FlagChangeEnum::Set:
            e.fields.doorFlags = change.flags;
            break;
        case FlagChangeEnum::Add:
            e.fields.doorFlags |= change.flags;
            break;
        case FlagChangeEnum::Remove:
            e.fields.doorFlags &= ~change.flags;
            break;
        }
        enforceInvariants(e);
    });
}

void World::apply(ProgressCounter &, const exit_change_types::SetDoorName &change)
{
    m_rooms.setExitDoorName(change.room, change.dir, change.name);
}

void World::apply(ProgressCounter &, const exit_change_types::ModifyExitFlags &change)
{
    const RoomId id = change.room;
    const ExitDirEnum dir = change.dir;

    requireValidRoom(id);

    switch (change.field.getType()) {
    case ExitFieldEnum::DOOR_NAME: {
        DoorName doorName = m_rooms.getExitDoorName(id, dir);
        applyDoorName(doorName, change.mode, change.field.getDoorName());
        m_rooms.setExitDoorName(id, dir, doorName);
        m_rooms.enforceInvariants(id, dir);
        return;
    }
    case ExitFieldEnum::EXIT_FLAGS: {
        auto flags = m_rooms.getExitExitFlags(id, dir);
        applyExitFlags(flags, change.mode, change.field.getExitFlags());
        m_rooms.setExitFlags_safe(id, dir, flags);
        return;
    }
    case ExitFieldEnum::DOOR_FLAGS: {
        auto flags = m_rooms.getExitDoorFlags(id, dir);
        applyDoorFlags(flags, change.mode, change.field.getDoorFlags());
        m_rooms.setExitDoorFlags(id, dir, flags);
        m_rooms.enforceInvariants(id, dir);
        return;
    }
    default:
        break;
    }
    std::abort();
}

void World::apply(ProgressCounter &, const room_change_types::AddPermanentRoom &change)
{
    const RoomId id = addRoom(change.position);
    setRoomStatus(id, RoomStatusEnum::Permanent);
}

void World::apply(ProgressCounter &, const room_change_types::AddRoom2 &change)
{
    addRoom2(change.position, change.event);
}

void World::apply(ProgressCounter &, const room_change_types::UndeleteRoom &change)
{
    undeleteRoom(change.room, change.raw);
}

void World::apply(ProgressCounter &, const room_change_types::RemoveRoom &change)
{
    removeFromWorld(change.room, true);
}

void World::apply(ProgressCounter &, const room_change_types::MakePermanent &change)
{
    m_rooms.setStatus(change.room, RoomStatusEnum::Permanent);
}

// FIXME: Yuck!
// TODO: Rewrite this as a series of apply()?
void World::apply(ProgressCounter & /*pc*/, const room_change_types::Update &change)
{
    RawRoom room = getRawCopy(change.room);
    const ParseEvent &event = change.event;

    room.fields.Contents = event.getRoomContents();

    const ConnectedRoomFlagsType connectedRoomFlags = event.getConnectedRoomFlags();
    ExitsFlagsType eventExitsFlags = event.getExitsFlags();
    if (eventExitsFlags.isValid()) {
        eventExitsFlags.removeValid();
        auto &exits = room.exits;

        if (true /*room.upToDate*/) {
            // Append exit flags if target room is up to date
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                RawExit &roomExit = exits[dir];
                const ExitFlags roomExitFlags = roomExit.fields.exitFlags;
                const ExitFlags eventExitFlags = eventExitsFlags.getWithUnmappedFlag(dir);

                if (eventExitFlags ^ roomExitFlags) {
                    roomExit.fields.exitFlags = (roomExitFlags | eventExitFlags);
                }

                const DoorFlags roomDoorFlags = roomExit.fields.doorFlags;
                const DoorFlags eventDoorFlags = event.getExits()[dir].fields.doorFlags;

                if (eventDoorFlags ^ roomDoorFlags) {
                    roomExit.fields.doorFlags = (roomDoorFlags | eventDoorFlags);
                }

                const auto &doorName = event.getExits()[dir].fields.doorName;
                if (!doorName.isEmpty()) {
                    roomExit.fields.doorName = doorName;
                }
            }

        } else {
            // Replace exit flags if target room is not up to date
            for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
                RawExit &roomExit = exits[dir];
                ExitFlags eventExitFlags = eventExitsFlags.getWithUnmappedFlag(dir);
                // ... but take care of the following exceptions

                // REVISIT: DOOR and EXIT flags are automatic now, so this may have no effect.
                if (true) {
                    if (roomExit.fields.exitFlags.contains(ExitFlagEnum::DOOR)
                        && !eventExitFlags.isDoor()) {
                        // Prevent room hidden exits from being overridden
                        eventExitFlags |= ExitFlagEnum::DOOR | ExitFlagEnum::EXIT;
                    }
                }
                if (roomExit.fields.exitFlags.contains(ExitFlagEnum::ROAD)
                    && !eventExitFlags.isRoad() && connectedRoomFlags.isValid()
                    && connectedRoomFlags.hasDirectSunlight(dir)) {
                    // Prevent orcs/trolls from removing roads/trails if they're sunlit
                    eventExitFlags |= ExitFlagEnum::ROAD;
                }
                roomExit.fields.exitFlags = eventExitFlags;
            }
        }
    }

    room.server_id = event.getServerId();

    room.fields.TerrainType = event.getTerrainType();

    const PromptFlagsType pFlags = event.getPromptFlags();
    if (pFlags.isValid()) {
        const RoomSundeathEnum sunType = room.fields.SundeathType;
        if (pFlags.isLit() && sunType == RoomSundeathEnum::NO_SUNDEATH) {
            room.fields.LightType = RoomLightEnum::LIT;
        } else if (pFlags.isDark() && sunType == RoomSundeathEnum::NO_SUNDEATH
                   && connectedRoomFlags.isValid() && connectedRoomFlags.hasAnyDirectSunlight()) {
            // REVISIT: Why not use the time and troll mode as well?
            room.fields.LightType = RoomLightEnum::DARK;
        }
    }

    const auto &desc = event.getRoomDesc();
    if (!desc.isEmpty()) {
        room.fields.Description = desc;
    }

    const auto &name = event.getRoomName();
    if (!name.isEmpty()) {
        room.fields.Name = name;
    }

    updateRoom(room);
}

void World::apply(ProgressCounter & /*pc*/, const room_change_types::SetServerId &change)
{
    //
    setServerId(change.room, change.server_id);
}

void World::apply(ProgressCounter & /*pc*/, const room_change_types::MoveRelative &change)
{
    //
    moveRelative(change.room, change.offset);
}

void World::apply(ProgressCounter & /*pc*/, const room_change_types::MoveRelative2 &change)
{
    //
    moveRelative(change.rooms, change.offset);
}

void World::apply(ProgressCounter & /*pc*/, const room_change_types::MergeRelative &change)
{
    //
    mergeRelative(change.room, change.offset);
}

namespace detail {
template<typename T, typename... /*Args*/>
class can_insert
{
    template<typename C, typename = decltype(std::declval<C &>() |= std::declval<const C &>())>
    static std::true_type test(int);
    template<typename C>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};
template<typename T, typename... /*Args*/>
class can_remove
{
    template<typename C, typename = decltype(std::declval<C &>() &= ~std::declval<const C &>())>
    static std::true_type test(int);
    template<typename C>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename T>
struct NODISCARD IsEnum final : std::bool_constant<std::is_enum_v<T>>
{};
static_assert(!IsEnum<RoomName>::value);
static_assert(IsEnum<RoomTerrainEnum>::value);
static_assert(!IsEnum<RoomMobFlags>::value);

template<typename T, typename...>
struct NODISCARD BasicSetUnsetHelper
{
    static inline constexpr bool isEnum = IsEnum<T>::value;
    static void sanitize(T &x)
    {
        if constexpr (isEnum) {
            if (!enums::isValidEnumValue(x)) {
                x = enums::sanitizeEnum(x);
            }
        }
    }

    static void assign(T &x, const T &change)
    {
        x = change;
        sanitize(x);
    }
    static void insert(T &x, const T &change)
    {
        // insert() is really only supported for flags
        assert(false);
        if constexpr (can_insert<T>::value) {
            x |= change;
            sanitize(x);
        } else {
            // cowardly refuse to do anything
        }
    }
    static void remove(T &x, const T &change)
    {
        // remove() is really only supported for flags
        assert(false);
        if constexpr (can_remove<T>::value) {
            x &= ~change;
            sanitize(x);
        } else {
            // cowardly refuse to do anything
        }
    }
    static void clear(T &x)
    {
        if constexpr (isEnum) {
            x = enums::getInvalidValue<T>();
        } else {
            x = T{};
        }
        sanitize(x);
    }
};

// clang-format off
//
// reason for disabling formatting: clion 2024.x mangles this struct every time you edit the file,
// even when clion is configured to use the project-wide .clang-format file.
template<typename T>
struct NODISCARD FlagSetUnsetHelper
{
    static void assign(T &x, const T &change)
    {
        x = change;
        sanityCheckFlags(x);
    }
    static void insert(T &x, const T &change)
    {
        x |= change;
        sanityCheckFlags(x);
    }
    static void remove(T &x, const T &change)
    {
        x &= ~change;
        sanityCheckFlags(x);
    }
    static void clear(T &x)
    {
        x.clear();
        sanityCheckFlags(x);
    }
};
// clang-format on

template<typename T>
struct IsTaggedString : std::bool_constant<false>
{};

template<typename T>
struct IsTaggedString<TaggedBoxedStringUtf8<T>> : std::bool_constant<true>
{};

static_assert(IsTaggedString<RoomName>::value);
static_assert(!IsTaggedString<RoomTerrainEnum>::value);
static_assert(!IsTaggedString<RoomMobFlags>::value);

template<typename T>
struct NODISCARD SetUnsetHelper final
    : std::conditional_t<IsTaggedString<T>::value || IsEnum<T>::value,
                         BasicSetUnsetHelper<T>,
                         FlagSetUnsetHelper<T>>
{
    static constexpr const bool isTagged = IsTaggedString<T>::value;
    static constexpr const bool isEnum = IsEnum<T>::value;
    static constexpr const bool isBasic = isTagged || isEnum;
};

static_assert(SetUnsetHelper<RoomName>::isBasic);
static_assert(SetUnsetHelper<RoomTerrainEnum>::isBasic);
static_assert(!SetUnsetHelper<RoomMobFlags>::isBasic);
static_assert(!SetUnsetHelper<RoomLoadFlags>::isBasic);

} // namespace detail

template<typename T>
static void applyChange(T &x, const T &change, const FlagModifyModeEnum mode)
{
    using Helper = detail::SetUnsetHelper<T>;
    switch (mode) {
    case FlagModifyModeEnum::ASSIGN:
        Helper::assign(x, change);
        break;
    case FlagModifyModeEnum::INSERT:
        Helper::insert(x, change);
        break;
    case FlagModifyModeEnum::REMOVE:
        Helper::remove(x, change);
        break;
    case FlagModifyModeEnum::CLEAR:
        Helper::clear(x);
        break;
    }
}

template<typename T>
NODISCARD static constexpr ParseKeyFlags getParseKeyFlags()
{
    return ParseKeyFlags{};
}

template<>
constexpr ParseKeyFlags getParseKeyFlags<RoomName>()
{
    return ParseKeyFlags{ParseKeyEnum::Name};
}

template<>
constexpr ParseKeyFlags getParseKeyFlags<RoomDesc>()
{
    return ParseKeyFlags{ParseKeyEnum::Desc};
}

void World::apply(ProgressCounter & /*pc*/, const room_change_types::ModifyRoomFlags &change)
{
#define X_SEP() else
#define X_VISIT(UPPER_CASE, CamelCase, Type) \
    if constexpr (std::is_same_v<Type, T>) { \
        const auto copy_before = m_rooms.getRoom##CamelCase(id); \
        auto field = copy_before; \
        applyChange<Type>(field, x, mode); \
        if (field != copy_before) { \
            /* a change will occur, so we'll see if we also need to modify parse keys */ \
            constexpr auto flags = getParseKeyFlags<Type>(); \
            if constexpr (flags) { \
                removeParse(id, flags); \
            } \
            /* this is the actual change */ \
            m_rooms.setRoom##CamelCase(id, field); \
            if constexpr (flags) { \
                insertParse(id, flags); \
            } \
        } \
    }
    /// The actual code
    {
        change.field.acceptVisitor([this, mode = change.mode, id = change.room](const auto &x) {
            using T = std::decay_t<decltype(x)>;
            // This expand to a long chain of if constexpr (...) else if constexpr (...)
            XFOREACH_ROOM_FIELD(X_VISIT, X_SEP)
            // and the abort is here to fail loudly if we screwed up the typecheck.
            else
            {
                std::abort();
            }
        });
    }
#undef X_VISIT
#undef X_SEP
}

void World::apply(ProgressCounter & /*pc*/, const room_change_types::TryMoveCloseTo &change)
{
    const RoomId id = change.room;
    const Coordinate &current = m_rooms.getPosition(id);
    const Coordinate &desired = change.desiredPosition;
    if (current == desired) {
        return;
    }

    auto check = [this, z = desired.z](const Coordinate &suggested) -> FindCoordEnum {
        if (suggested.z == z && !findRoom(suggested)) {
            return FindCoordEnum::Available;
        } else {
            return FindCoordEnum::InUse;
        }
    };

    const Coordinate assigned = getNearestFree(desired, check);
    setPosition(id, assigned);
}

void World::post_change_updates(ProgressCounter &pc)
{
    if (needsBoundsUpdate()) {
        updateBounds(pc);
    }
    checkConsistency(pc);
}

namespace {
struct NODISCARD WorldChangePrinter final
{
private:
    const World &m_world;
    AnsiOstream &m_aos;
    ChangePrinter m_cp;

public:
    explicit WorldChangePrinter(const World &w, AnsiOstream &aos)
        : m_world{w}
        , m_aos{aos}
        , m_cp{[this](RoomId id) -> ExternalRoomId {
                   if (!m_world.hasRoom(id)) {
                       return INVALID_EXTERNAL_ROOMID;
                   }
                   return m_world.convertToExternal(id);
               },
               aos}
    {}
    void print(const Change &change) { m_cp.print(change); }
    void print(const std::vector<Change> &changes, const std::string_view sep)
    {
        size_t num_printed = 0;
        std::string_view prefix = "";
        for (const Change &change : changes) {
            m_aos << prefix;
            prefix = sep;
            if (num_printed++ >= max_change_batch_print_size) {
                m_aos.writeWithColor(getRawAnsi(AnsiColor16Enum::RED),
                                     "...(change list print limit reached)...");
                break;
            }
            m_cp.print(change);
        }
    }
};

} // namespace

void World::printChange(AnsiOstream &aos, const Change &change) const
{
    WorldChangePrinter{*this, aos}.print(change);
}

void World::printChanges(AnsiOstream &aos,
                         const std::vector<Change> &changes,
                         const std::string_view sep) const
{
    WorldChangePrinter{*this, aos}.print(changes, sep);
}

void World::applyOne(ProgressCounter &pc, const Change &change)
{
    if (print_world_changes) {
        std::ostringstream oss;
        {
            AnsiOstream aos{oss};
            aos << "[world] Applying 1 change...\n";
            printChange(aos, change);
            oss << "\n";
        }
        MMLOG_INFO() << oss.str();
    }
    change.acceptVisitor([this, &pc](const auto &specialized_change) {
        //
        this->apply(pc, specialized_change);
    });
    post_change_updates(pc);
}

void World::applyAll(ProgressCounter &pc, const std::vector<Change> &changes)
{
    applyAll_internal(pc, changes);
    post_change_updates(pc);
}

void World::zapRooms_unsafe(ProgressCounter &pc, const RoomIdSet &rooms)
{
    DECL_TIMER(t, __FUNCTION__);

    std::vector<exit_change_types::ModifyExitConnection> removals;
    auto sched_removal = [&removals](RoomId from, ExitDirEnum dir, RoomId to) {
        removals.emplace_back(exit_change_types::ModifyExitConnection{ChangeTypeEnum::Remove,
                                                                      from,
                                                                      dir,
                                                                      to,
                                                                      WaysEnum::TwoWay});
    };
    {
        DECL_TIMER(t2, "finding inbound exits");
        pc.increaseTotalStepsBy(getRoomSet().size());
        for (const RoomId id : getRoomSet()) {
            for (const ExitDirEnum dir : ALL_EXITS7) {
                const ExitDirEnum rev = opposite(dir);
                for (const RoomId to : m_rooms.getExitOutgoing(id, dir)) {
                    if (!rooms.contains(to)) {
                        sched_removal(id, dir, to);
                    }
                }
                for (const RoomId from : m_rooms.getExitIncoming(id, dir)) {
                    if (!rooms.contains(from)) {
                        sched_removal(from, rev, id);
                    }
                }
            }
            pc.step();
        }
    }

    {
        DECL_TIMER(t2, "zapping inbound exits");
        pc.increaseTotalStepsBy(removals.size());
        for (const exit_change_types::ModifyExitConnection &rem : removals) {
            apply(pc, rem);
            pc.step();
        }
    }

    {
        DECL_TIMER(t2, "zapping rooms");
        pc.increaseTotalStepsBy(rooms.size());
        for (const RoomId room : rooms) {
            removeFromWorld(room, false);
            pc.step();
        }
    }
}

void World::applyAll_internal(ProgressCounter &pc, const std::vector<Change> &changes)
{
    DECL_TIMER(t, __FUNCTION__);

    if (changes.empty()) {
        throw InvalidMapOperation("Changes are empty");
    }

    if (print_world_changes) {
        std::ostringstream oss;
        {
            AnsiOstream aos{oss};
            const auto count = changes.size();
            aos << "[world] Applying " << count << " change" << ((count == 1) ? "" : "s")
                << "...\n";
            printChanges(aos, changes, "\n");
            aos << "\n";
        }
        MMLOG_INFO() << oss.str();
    }

    pc.increaseTotalStepsBy(changes.size());
    for (const Change &change : changes) {
        change.acceptVisitor([this, &pc](const auto &x) {
            //
            this->apply(pc, x);
        });
        pc.step();
    }
}

void World::printStats(ProgressCounter &pc, AnsiOstream &os) const
{
    m_remapping.printStats(pc, os);
    m_serverIds.printStats(pc, os);

    {
        size_t numMissingName = 0;
        size_t numMissingDesc = 0;
        size_t numMissingBoth = 0;
        size_t numMissingServerId = 0;
        size_t numWithNoConnections = 0;
        size_t numWithNoExits = 0;
        size_t numWithNoEntrances = 0;
        size_t numExits = 0;
        size_t numDoors = 0;
        size_t numHidden = 0;
        size_t numDoorNames = 0;
        size_t numHiddenDoorNames = 0;
        size_t numLoopExits = 0;
        size_t numConnections = 0;
        size_t numMultipleOut = 0;
        size_t numMultipleIn = 0;

        size_t adj1 = 0;
        size_t adj2 = 0;
        size_t non1 = 0;
        size_t non2 = 0;
        size_t loop1 = 0;
        size_t loop2 = 0;

        std::optional<Bounds> optBounds;
        for (const RoomId id : getRoomSet()) {
            const RawRoom *const pRoom = getRoom(id);
            if (pRoom == nullptr) {
                std::abort();
            }
            auto &room = *pRoom;
            const auto &pos = getPosition(id);

            if (optBounds.has_value()) {
                optBounds->insert(pos);
            } else {
                optBounds.emplace(pos, pos);
            }

            const bool isMissingServerId = room.getServerId() == INVALID_SERVER_ROOMID;
            if (isMissingServerId) {
                ++numMissingServerId;
            }

            const bool isMissingName = getRoomName(id).empty();
            const bool isMissingDesc = getRoomDescription(id).empty();

            if (isMissingName) {
                ++numMissingName;
            }

            if (isMissingDesc) {
                ++numMissingDesc;
            }

            if (isMissingName && isMissingDesc) {
                ++numMissingBoth;
            }

            bool hasExits = false;
            bool hasEntrances = false;

            for (const ExitDirEnum dir : ALL_EXITS7) {
                const auto &e = getExitExitFlags(id, dir);

                if (e.isExit()) {
                    ++numExits;
                }

                if (e.isDoor()) {
                    ++numDoors;
                    if (!getExitDoorName(id, dir).empty()) {
                        ++numDoorNames;
                    }
                }

                if (getExitDoorFlags(id, dir).isHidden()) {
                    numHidden += 1;
                    if (!getExitDoorName(id, dir).empty()) {
                        ++numHiddenDoorNames;
                    }
                }

                const TinyRoomIdSet &outset = m_rooms.getExitOutgoing(id, dir);
                const TinyRoomIdSet &inset = m_rooms.getExitIncoming(id, dir);

                if (!outset.empty()) {
                    hasExits = true;
                }

                if (!inset.empty()) {
                    hasEntrances = true;
                }

                const auto outSize = outset.size();
                numConnections += outSize;

                if (outSize > 1) {
                    ++numMultipleOut;
                }

                if (inset.size() > 1) {
                    ++numMultipleIn;
                }

                if (outset.contains(id)) {
                    ++numLoopExits;
                }

                const ExitDirEnum rev = opposite(dir);
                for (const RoomId to : outset) {
                    if (hasRoom(to)) {
                        const bool looping = (id == to);
                        const bool adjacent = pos + exitDir(dir) == getPosition(to);
                        const bool twoWay = getOutgoing(to, rev).contains(id);

                        if (looping) {
                            (twoWay ? loop2 : loop1) += 1;
                        } else if (adjacent) {
                            (twoWay ? adj2 : adj1) += 1;
                        } else {
                            (twoWay ? non2 : non1) += 1;
                        }
                    }
                }
            }

            if (!hasEntrances && !hasExits) {
                ++numWithNoConnections;
            }

            if (!hasEntrances) {
                ++numWithNoEntrances;
            }

            if (!hasExits) {
                ++numWithNoExits;
            }
        }

        os << "\n";
        os << "Total rooms: " << m_roomSet.size() << ".\n";
        os << "\n";
        os << "  missing server id: " << numMissingServerId << ".\n";
        os << "\n";
        os << "  with no name and no desc: " << numMissingBoth << ".\n";
        os << "  with name but no desc:    " << (numMissingDesc - numMissingBoth) << ".\n";
        os << "  with desc but no name:    " << (numMissingName - numMissingBoth) << ".\n";
        os << "\n";
        os << "  with no connections:         " << numWithNoConnections << ".\n";
        os << "  with entrances but no exits: " << (numWithNoExits - numWithNoConnections) << ".\n";
        os << "  with exits but no entrances: " << (numWithNoEntrances - numWithNoConnections)
           << ".\n";
        os << "\n";
        os << "Total exits: " << numExits << ".\n";
        os << "\n";
        os << "  doors:  " << numDoors << " (with names: " << numDoorNames << ").\n";
        os << "  hidden: " << numHidden << " (with names: " << numHiddenDoorNames << ").\n";
        os << "  loops:  " << numLoopExits << ".\n";
        os << "\n";
        os << "  with multiple outputs: " << numMultipleOut << ".\n";
        os << "  with multiple inputs:  " << numMultipleIn << ".\n";
        os << "\n";
        os << "Total connections: " << numConnections << ".\n";
        os << "\n";
        os << "  adjacent 1-way:     " << adj1 << ".\n";
        os << "  adjacent 2-way:     " << adj2 << ".\n";
        os << "  looping 1-way:      " << loop1 << ".\n";
        os << "  looping 2-way:      " << loop2 << ".\n";
        os << "  non-adjacent 1-way: " << non1 << ".\n";
        os << "  non-adjacent 2-way: " << non2 << ".\n";
        os << "\n";
        os << "  total 1-way:        " << (non1 + adj1 + loop1) << ".\n";
        os << "  total 2-way:        " << (non2 + adj2 + loop2) << ".\n";
        os << "  total adjacent:     " << (adj1 + adj2) << ".\n";
        os << "  total looping:      " << (loop1 + loop2) << ".\n";
        os << "  total non-adjacent: " << (non1 + non2 + loop1 + loop2) << ".\n";
    }

    m_spatialDb.printStats(pc, os);
    m_parseTree.printStats(pc, os);
}

bool World::isTemporary(const RoomId id) const
{
    requireValidRoom(id);
    return m_rooms.getStatus(id) == RoomStatusEnum::Temporary;
}

#define X_DEFINE_GETTER(Type, Name, Init) \
    Type World::getRoom##Name(RoomId id) const \
    { \
        requireValidRoom(id); \
        return m_rooms.getRoom##Name(id); \
    }
XFOREACH_ROOM_PROPERTY(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

bool World::containsRoomsNotIn(const World &other) const
{
    return m_roomSet.containsElementNotIn(other.m_roomSet);
}

namespace { // anonymous

NODISCARD bool hasMeshDifference(const RawExit &a, const RawExit &b)
{
    // door name change is not a mesh difference
    return a.fields.exitFlags != b.fields.exitFlags     //
           || a.fields.doorFlags != b.fields.doorFlags; //
}

NODISCARD bool hasMeshDifference(const RawRoom::Exits &a, const RawRoom::Exits &b)
{
    for (auto dir : ALL_EXITS7) {
        if (hasMeshDifference(a[dir], b[dir])) {
            return true;
        }
    }
    return false;
}

NODISCARD bool hasMeshDifference(const RoomFields &a, const RoomFields &b)
{
#define X_CASE(_Type, _Name, _Init) \
    if ((a._Name) != (b._Name)) { \
        return true; \
    }
    // NOTE: Purposely *NOT* doing "XFOREACH_ROOM_STRING_PROPERTY(X_CASE)"
    XFOREACH_ROOM_FLAG_PROPERTY(X_CASE)
    XFOREACH_ROOM_ENUM_PROPERTY(X_CASE)
    return false;
#undef X_CASE
}

NODISCARD bool hasMeshDifference(const RawRoom &a, const RawRoom &b)
{
    return a.position != b.position                 //
           || hasMeshDifference(a.fields, b.fields) //
           || hasMeshDifference(a.exits, b.exits);  //
}

// Only valid if one is immediately derived from the other.
NODISCARD bool hasMeshDifference(const World &a, const World &b)
{
    for (const RoomId id : a.getRoomSet()) {
        if (!b.hasRoom(id)) {
            // technically we could return true here, but the function assumes that it won't be
            // called if the worlds added or removed any rooms, so we only care about common rooms.
            continue;
        }
        if (hasMeshDifference(deref(a.getRoom(id)), deref(b.getRoom(id)))) {
            return true;
        }
    }
    return false;
}
} // namespace

// Only valid if one is immediately derived from the other.
WorldComparisonStats World::getComparisonStats(const World &base, const World &modified)
{
    const auto anyRoomsAdded = modified.containsRoomsNotIn(base);
    const auto anyRoomsRemoved = base.containsRoomsNotIn(modified);
    const auto anyRoomsMoved = base.m_spatialDb != modified.m_spatialDb;

    WorldComparisonStats result;
    result.boundsChanged = base.getBounds() != modified.getBounds();
    result.anyRoomsRemoved = anyRoomsRemoved;
    result.anyRoomsAdded = anyRoomsAdded;
    result.spatialDbChanged = anyRoomsMoved;
    result.serverIdsChanged = base.m_serverIds != modified.m_serverIds;
    result.parseTreeChanged = base.m_parseTree != modified.m_parseTree;
    result.hasMeshDifferences = anyRoomsAdded                         //
                                || anyRoomsRemoved                    //
                                || anyRoomsMoved                      //
                                || hasMeshDifference(base, modified); //

    return result;
}
