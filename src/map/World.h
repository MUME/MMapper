#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/Timer.h"
#include "../global/macros.h"
#include "Changes.h"
#include "ExitFields.h"
#include "InvalidMapOperation.h"
#include "ParseTree.h"
#include "RawRooms.h"
#include "Remapping.h"
#include "ServerIdMap.h"
#include "SpatialDb.h"
#include "WorldAreaMap.h"

#include <memory>
#include <optional>
#include <ostream>
#include <thread>
#include <vector>

class RawRooms;

struct NODISCARD WorldComparisonStats final
{
    bool boundsChanged = false;
    bool anyRoomsRemoved = false;
    bool anyRoomsAdded = false;

    bool spatialDbChanged = false;
    bool serverIdsChanged = false;

    bool hasMeshDifferences = false;
};

class NODISCARD World final
{
private:
    Remapping m_remapping;
    RawRooms m_rooms;
    /// This must be updated any time a room's position changes.
    SpatialDb m_spatialDb;
    ServerIdMap m_serverIds;
    ParseTree m_parseTree;
    AreaInfoMap m_areaInfos;
    bool m_checkedConsistency = false;

public:
    explicit World() = default;
    ~World() = default;
    World(World &&) = default;
    World(const World &) = delete;
    World &operator=(World &&) = delete;
    World &operator=(const World &) = delete;

public:
    NODISCARD World copy() const;

public:
    NODISCARD bool operator==(const World &rhs) const;
    NODISCARD bool operator!=(const World &rhs) const { return !(rhs == *this); }

private:
    NODISCARD AreaInfo *findArea(const std::optional<RoomArea> &area);
    NODISCARD const AreaInfo *findArea(const std::optional<RoomArea> &area) const;
    NODISCARD AreaInfo &getArea(const std::optional<RoomArea> &area);
    NODISCARD const AreaInfo &getArea(const std::optional<RoomArea> &area) const;

    NODISCARD AreaInfo &getGlobalArea() { return getArea(std::nullopt); }
    NODISCARD const AreaInfo &getGlobalArea() const { return getArea(std::nullopt); }

public:
    NODISCARD const ParseTree &getParseTree() const { return m_parseTree; }

public:
    NODISCARD const RawRoom *getRoom(RoomId id) const;

public:
    NODISCARD std::optional<Bounds> getBounds() const { return m_spatialDb.getBounds(); }
    NODISCARD bool needsBoundsUpdate() const { return m_spatialDb.needsBoundsUpdate(); }
    void updateBounds(ProgressCounter &pc) { m_spatialDb.updateBounds(pc); }

public:
    NODISCARD RoomId getNextId() const;
    // WARNING: This is not cheap.
    NODISCARD ExternalRoomId getNextExternalId() const;

public:
    NODISCARD const RoomIdSet &getRoomSet() const;
    NODISCARD const RoomIdSet *findAreaRoomSet(const RoomArea &areaName) const;

public:
    NODISCARD bool hasRoom(RoomId id) const;
    void requireValidRoom(RoomId id) const;

public:
    NODISCARD std::optional<RoomId> findRoom(const Coordinate &coord) const;
    NODISCARD const Coordinate &getPosition(RoomId id) const;

public:
    NODISCARD ServerRoomId getServerId(RoomId id) const;
    NODISCARD std::optional<RoomId> lookup(ServerRoomId id) const;

public:
#define X_DECL_ACCESSORS(Type, Name, Init) \
    NODISCARD const Type &getExit##Type(RoomId id, ExitDirEnum dir) const;
    XFOREACH_EXIT_FLAG_PROPERTY(X_DECL_ACCESSORS)
#undef X_DECL_ACCESSORS

    NODISCARD DoorName getExitDoorName(RoomId id, ExitDirEnum dir) const;

    NODISCARD bool hasExit(RoomId id, ExitDirEnum dir) const;

public:
#define X_DEFINE_GETTER(_Type, _Prop, _OptInit) \
    NODISCARD _Type get##_Type(RoomId id, ExitDirEnum dir) const;
    XFOREACH_EXIT_PROPERTY(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

#define X_DEFINE_GETTER(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool exitIs##CamelCase(RoomId room, ExitDirEnum dir) const;
    XFOREACH_EXIT_FLAG(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

#define X_DEFINE_GETTER(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD bool doorIs##CamelCase(RoomId room, ExitDirEnum dir) const;
    XFOREACH_DOOR_FLAG(X_DEFINE_GETTER)
#undef X_DEFINE_GETTER

public:
    NODISCARD const TinyRoomIdSet &getOutgoing(RoomId id, ExitDirEnum dir) const;
    NODISCARD const TinyRoomIdSet &getIncoming(RoomId id, ExitDirEnum dir) const;

public:
    NODISCARD bool hasConsistentOneWayExit(RoomId from, ExitDirEnum dir, RoomId to) const;
    NODISCARD bool hasConsistentTwoWayExit(RoomId from, ExitDirEnum dir, RoomId to) const;
    NODISCARD bool hasConsistentExit(RoomId from, ExitDirEnum dir, RoomId to, WaysEnum ways) const;

public:
    void checkConsistency(ProgressCounter &counter) const;

public:
    NODISCARD RawExit getRawExit(RoomId id, ExitDirEnum dir) const;
    NODISCARD RawRoom getRawCopy(RoomId id) const;

public:
    NODISCARD static World init(ProgressCounter &counter, const std::vector<ExternalRawRoom> &map);

public:
    NODISCARD ExternalRoomIdSet convertToExternal(ProgressCounter &pc,
                                                  const TinyRoomIdSet &room) const;
    NODISCARD ExternalRawExit convertToExternal(const RawExit &exit) const;
    NODISCARD ExternalRawRoom convertToExternal(const RawRoom &room) const;

    NODISCARD RoomId convertToInternal(ExternalRoomId ext) const;
    NODISCARD ExternalRoomId convertToExternal(RoomId id) const;

public:
    void applyOne(ProgressCounter &pc, const Change &change);
    void applyAll(ProgressCounter &pc, const std::vector<Change> &changes);

public:
    NODISCARD bool isTemporary(RoomId id) const;

public:
#define X_DECL_GETTER(Type, Name, Init) NODISCARD Type getRoom##Name(RoomId id) const;
    XFOREACH_ROOM_PROPERTY(X_DECL_GETTER)
#undef X_DECL_GETTER

public:
    void printStats(ProgressCounter &pc, AnsiOstream &aos) const;
    NODISCARD static WorldComparisonStats getComparisonStats(const World &base,
                                                             const World &modified);

private:
    void insertParse(RoomId id, ParseKeyFlags parseKeys);

    /// Note: This must be done before you modify the room's name, desc, or prompt flags!
    void removeParse(RoomId id, ParseKeyFlags parseKeys);

private:
    NODISCARD static ParseKeyFlags parseKeysChanged(const RawRoom &a, const RawRoom &b);

private:
    void setRoom(RoomId id, const RawRoom &room);

private:
    NODISCARD bool hasOneWayExit_inconsistent(RoomId from,
                                              ExitDirEnum dir,
                                              InOutEnum mode,
                                              RoomId to) const;

    NODISCARD bool hasTwoWayExit_inconsistent(RoomId from,
                                              ExitDirEnum dir,
                                              InOutEnum mode,
                                              RoomId to) const;

private:
    void addExit_inconsistent(RoomId from, ExitDirEnum dir, InOutEnum mode, RoomId to);

private:
    void addConsistentOneWayExit(RoomId from, ExitDirEnum dir, RoomId to);
    void addExit(RoomId from, ExitDirEnum dir, RoomId to, WaysEnum ways);

private:
    void removeExit_inconsistent(RoomId from, ExitDirEnum dir, InOutEnum mode, RoomId to);
    void removeExit_consistently(RoomId from, ExitDirEnum dir, RoomId to);

private:
    void removeExit(RoomId from, ExitDirEnum dir, RoomId to, WaysEnum ways);

private:
    void checkAllExitsConsistent(RoomId id) const;

private:
    void clearExit(RoomId id, ExitDirEnum dir, WaysEnum ways);
    void nukeHelper(RoomId id, ExitDirEnum dir, const RawExit &ex, WaysEnum ways);

private:
    void nukeExit(RoomId id, ExitDirEnum dir, WaysEnum ways);
    void nukeAllExits(RoomId id, WaysEnum ways);

private:
    void moveRelative(RoomId id, const Coordinate &offset);
    void moveRelative(const RoomIdSet &rooms, const Coordinate &offset);
    void setPosition(RoomId id, const Coordinate &coord);
    void setServerId(RoomId id, ServerRoomId serverId);

private:
    void removeFromWorld(RoomId id, bool removeLinks);

private:
    void updateRoom(const RawRoom &room);

private:
    void copyStatusAndExitFields(const RawRoom &from);
    void setRoomStatus(RoomId id, RoomStatusEnum status);
    void setRoomExitFields(RoomId id, ExitDirEnum dir, const ExitFields &fields);

private:
    // for historical reasons, this only allows edits to status and exit fields.
    template<typename Callback>
    void apply_update(RoomId id, Callback &&callback)
    {
        // function signature check: void(Room&)
        static_assert(std::is_invocable_r_v<void, Callback, RawRoom &>);

        // copy prevents callback from modifying disallowed fields
        RawRoom raw = getRawCopy(id);
        callback(raw);

        // only allows status or exit fields (DoorFlags, DoorName, ExitFlags).
        copyStatusAndExitFields(raw);
    }

private:
    static void merge_update(RawRoom &target, const RawRoom &source);
    void copy_exits(RoomId targetId, const RawRoom &source);

private:
    void mergeRelative(RoomId id, const Coordinate &offset);

private:
    void setRemapAndAllocateRooms(Remapping new_remap);
    void setExit(RoomId id, ExitDirEnum dir, const RawExit &input);
    void setRoom_lowlevel(RoomId id, const RawRoom &input);
    void initRoom(const RawRoom &input);

private:
    NODISCARD RoomId addRoom(const Coordinate &position);
    void addRoom2(const Coordinate &desiredPosition, const ParseEvent &event);
    void undeleteRoom(ExternalRoomId id, const RawRoom &raw);

private:
    void zapRooms_unsafe(ProgressCounter &pc, const RoomIdSet &rooms);

private:
#define X_NOP()
#define X_DECL_APPLY(_Type) void apply(ProgressCounter &pc, const _Type &change);
    XFOREACH_CHANGE_TYPE(X_DECL_APPLY, X_NOP)
#undef X_DECL_APPLY
#undef X_NOP

private:
    void post_change_updates(ProgressCounter &pc);
    void applyAll_internal(ProgressCounter &pc, const std::vector<Change> &changes);

private:
    NODISCARD bool containsRoomsNotIn(const World &other) const;

public:
    NODISCARD bool wouldAllowRelativeMove(const RoomIdSet &set, const Coordinate &offset) const;

public:
    void printChange(AnsiOstream &aos, const Change &change) const;
    void printChanges(AnsiOstream &aos,
                      const std::vector<Change> &changes,
                      std::string_view sep) const;
};
