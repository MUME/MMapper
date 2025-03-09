#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "RawRoom.h"
#include "RoomFieldVariant.h"
#include "RoomIdSet.h"
#include "coordinate.h"
#include "mmapper2room.h"
#include "parseevent.h"
#include "room.h"
#include "roomid.h"

#include <memory>
#include <optional>
#include <ostream>
#include <vector>

class AnsiOstream;
class Change;
class ChangeList;
class ProgressCounter;
class RoomHandle;
class RoomRecipient;
class World;
struct MapApplyResult;
struct MapPair;

class NODISCARD Map final
{
private:
    friend RoomHandle;
    std::shared_ptr<const World> m_world;

public:
    Map();
    explicit Map(std::shared_ptr<const World> world);
    explicit Map(World world);

public:
    NODISCARD bool isSamePointer(const Map &other) const
    {
        return &getWorld() == &other.getWorld();
    }

public:
    NODISCARD size_t getRoomsCount() const;
    NODISCARD bool empty() const { return getRoomsCount() == 0; }
    NODISCARD std::optional<Bounds> getBounds() const;

public:
    NODISCARD const RoomIdSet &getRooms() const;
    DEPRECATED_MSG("use findAllRooms()")
    void getRooms(RoomRecipient &recipient, const ParseEvent &) const;
    NODISCARD RoomIdSet findAllRooms(const ParseEvent &) const;

private:
    NODISCARD const RawRoom *find_room_ptr(RoomId id) const;

public:
    // Semantics: "findRoomHandle()" functions can return an invalid handle;
    // maybe these should be called "lookup()"?
    NODISCARD RoomHandle findRoomHandle(RoomId id) const;
    NODISCARD RoomHandle findRoomHandle(ExternalRoomId id) const;
    NODISCARD RoomHandle findRoomHandle(ServerRoomId id) const;
    NODISCARD RoomHandle findRoomHandle(const Coordinate &coord) const;

public:
    // Semantics: "getRoomHandle()" functions throw if the handle is invalid;
    // use these when you demand that the value must exist.
    // It would probably help to have a better name distinction than find() vs get().
    NODISCARD RoomHandle getRoomHandle(RoomId id) const;
    NODISCARD RoomHandle getRoomHandle(ExternalRoomId id) const;

public:
    NODISCARD const RawRoom &getRawRoom(RoomId id) const;

public:
    NODISCARD std::optional<DoorName> findDoorName(RoomId id, ExitDirEnum dir) const;

public:
    NODISCARD bool operator==(const Map &other) const;
    NODISCARD bool operator!=(const Map &other) const { return !operator==(other); }

public:
    NODISCARD MapApplyResult apply(ProgressCounter &pc, const std::vector<Change> &changes) const;
    NODISCARD MapApplyResult apply(ProgressCounter &pc, const ChangeList &changes) const;
    NODISCARD MapApplyResult applySingleChange(ProgressCounter &pc, const Change &change) const;

public:
    NODISCARD Map filterBaseMap(ProgressCounter &pc) const;

public:
    NODISCARD const World &getWorld() const { return *m_world; }

public:
    NODISCARD static MapPair fromRooms(ProgressCounter &counter, std::vector<ExternalRawRoom> rooms);
    void printMulti(ProgressCounter &pc, AnsiOstream &aos) const;
    void printStats(ProgressCounter &pc, AnsiOstream &aos) const;
    void printUnknown(ProgressCounter &pc, AnsiOstream &aos) const;

    // NOTE: This throws if there's a failure!
    void checkConsistency(ProgressCounter &counter) const;

public:
    void statRoom(AnsiOstream &os, RoomId id) const;

public:
    static void diff(ProgressCounter &pc, AnsiOstream &os, const Map &a, const Map &b);

public:
    NODISCARD size_t countRoomsWithName(const RoomName &name) const;
    NODISCARD size_t countRoomsWithDesc(const RoomDesc &desc) const;
    NODISCARD size_t countRoomsWithNameDesc(const RoomName &name, const RoomDesc &desc) const;
    NODISCARD std::optional<RoomId> findUniqueName(const RoomName &name) const;
    NODISCARD std::optional<RoomId> findUniqueDesc(const RoomDesc &desc) const;
    NODISCARD std::optional<RoomId> findUniqueNameDesc(const RoomName &name,
                                                       const RoomDesc &desc) const;

public:
    NODISCARD bool hasUniqueName(RoomId id) const;
    NODISCARD bool hasUniqueDesc(RoomId id) const;
    NODISCARD bool hasUniqueNameDesc(RoomId id) const;

    NODISCARD ExternalRoomId getExternalRoomId(RoomId id) const;

public:
    NODISCARD static Map merge(ProgressCounter &pc,
                               const Map &currentMap,
                               std::vector<ExternalRawRoom> newRooms,
                               const Coordinate &mapOffset);

    static void foreachChangedRoom(ProgressCounter &pc,
                                   const Map &saved,
                                   const Map &current,
                                   const std::function<void(const RawRoom &room)> &callback);
};

struct NODISCARD MapApplyResult final
{
    Map map;
    RoomUpdateFlags roomUpdateFlags = ~RoomUpdateFlags{};
};

struct NODISCARD MapPair final
{
    Map base;
    Map modified;
};

struct NODISCARD BasicDiffStats final
{
    size_t numRoomsRemoved = 0;
    size_t numRoomsAdded = 0;
    size_t numRoomsChanged = 0;
};

NODISCARD extern BasicDiffStats getBasicDiffStats(const Map &base, const Map &modified);

void displayRoom(AnsiOstream &os, const RoomHandle &r, RoomFieldFlags fieldset);
/* normally only called by displayExits */
void enhanceExits(AnsiOstream &os, const RoomHandle &sourceRoom);
void displayExits(AnsiOstream &os, const RoomHandle &r, char sunCharacter);
void previewRoom(AnsiOstream &os, const RoomHandle &r);
NODISCARD std::string previewRoom(const RoomHandle &r);

namespace mmqt {
enum class NODISCARD StripAnsiEnum : uint8_t { No, Yes };
enum class NODISCARD PreviewStyleEnum : uint8_t { ForLog, ForDisplay };
NODISCARD QString previewRoom(const RoomHandle &room,
                              StripAnsiEnum stripAnsi,
                              PreviewStyleEnum previewStyle);
} // namespace mmqt

namespace test {
extern void testMap();
} // namespace test
