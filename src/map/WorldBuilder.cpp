// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "WorldBuilder.h"

#include "../global/Consts.h"
#include "../global/Timer.h"
#include "../global/logging.h"
#include "../global/progresscounter.h"
#include "InvalidMapOperation.h"
#include "Remapping.h"
#include "World.h"

#include <map>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

using char_consts::C_NEWLINE;

NODISCARD static TinyExternalRoomIdSet &getInOut(ExternalRawExit &exit, InOutEnum mode)
{
    return (mode == InOutEnum::In) ? exit.incoming : exit.outgoing;
}

SanitizerChanges WorldBuilder::sanitize(ProgressCounter &counter,
                                        std::vector<ExternalRawRoom> &input)
{
    DECL_TIMER(t0, "sanitize");
    counter.setCurrentTask(ProgressMsg{"sanitizing input"});

    {
        size_t roomsChanged = 0;
        counter.increaseTotalStepsBy(input.size());
        for (ExternalRawRoom &raw : input) {
            auto copy = raw;
            ::sanitize(raw);
            if (copy != raw) {
                ++roomsChanged;
            }
            counter.step();
        }

        MMLOG() << "[sanitize] updated fields in " << roomsChanged << " room"
                << ((roomsChanged == 1) ? "" : "s") << ".";
    }

    struct NODISCARD LookupTable final : std::map<ExternalRoomId, uint32_t>
    {
        NODISCARD bool contains(const ExternalRoomId id) const { return find(id) != end(); }
    };

    const LookupTable lookupTable = [&input]() -> LookupTable {
        DECL_TIMER(t2, "build-lookup-table");
        LookupTable roomMap;
        const auto inputSize = static_cast<uint32_t>(input.size());
        for (uint32_t i = 0; i < inputSize; ++i) {
            const auto &r = input[i];
            if (r.id == INVALID_EXTERNAL_ROOMID || roomMap.contains(r.id)) {
                throw InvalidMapOperation();
            }
            roomMap[r.id] = i;
        }
        return roomMap;
    }();

    auto checkAndRepairExits = [&input, &lookupTable](ExternalRawRoom &r,
                                                      const ExitDirEnum dir,
                                                      const InOutEnum mode,
                                                      size_t &addedMissing,
                                                      size_t &removedInvalid) {
        const auto rev = opposite(dir);
        const auto otherMode = opposite(mode);

        auto &exit = r.exits[dir];
        auto &set = getInOut(exit, mode);

        ExternalRoomIdSet remove_set;
        for (const ExternalRoomId otherId : set) {
            if (!lookupTable.contains(otherId)) {
                // Note: Can't modify the set we're iterating.
                remove_set.insert(otherId);
                continue;
            }

            const auto idx = lookupTable.at(otherId);
            ExternalRawRoom &other = input.at(idx);
            assert(other.id == otherId);

            auto &otherEx = other.exits[rev];
            auto &otherSet = getInOut(otherEx, otherMode);

            if (otherSet.contains(r.id)) {
                continue;
            }

            // NOTE: It's always safe to modify the other set, even if it's the same room,
            // because it's the opposite in/out of the one we're iterating.
            //
            // This pedantic assertion will never fire.
            assert(std::addressof(set) != std::addressof(otherSet));

            otherSet.insert(r.id);
            ++addedMissing;
        }

        if (!remove_set.empty()) {
            for (const ExternalRoomId other : remove_set) {
                assert(set.contains(other));
                set.erase(other);
                ++removedInvalid;
            }
        }
    };

    struct NODISCARD CoordSet final : public std::unordered_set<Coordinate>
    {
        NODISCARD bool contains(const Coordinate &c) const { return find(c) != end(); }
    };

    auto hasUniqueCoords = [&input]() -> bool {
        CoordSet seenCoords;
        for (const auto &r : input) {
            auto pos = r.getPosition();
            if (seenCoords.contains(pos)) {
                return false;
            }
            seenCoords.insert(pos);
        }
        return true;
    };

    SanitizerChanges output;
    size_t numRepairedCoords = 0;

    auto checkAndRepairCoords = [&input, &output, &lookupTable, &numRepairedCoords]() {
        Bounds bounds;
        CoordSet seenCoords;
        std::set<ExternalRoomId> needsNewCoordinate;
        for (const auto &r : input) {
            auto pos = r.getPosition();
            bounds.insert(pos);
            if (seenCoords.contains(pos)) {
                // abnormal
                needsNewCoordinate.insert(r.getId());
            } else {
                // normal
                seenCoords.insert(pos);
            }
        }

        if (needsNewCoordinate.empty()) {
            return;
        }

        const auto lo = bounds.min.x;
        const auto hi = bounds.max.x;
        Coordinate cursor{lo, bounds.max.y + 1, 0};
        auto assignUnusedPosition = [&cursor, &lo, &hi, &seenCoords](const int z) -> Coordinate {
            auto result = cursor;
            if (cursor.x == hi) {
                cursor.x += 1;
            } else {
                cursor.x = lo;
                cursor.y += 1;
            }

            result.z = z;

            if (seenCoords.contains(result)) {
                std::abort();
            }

            seenCoords.insert(result);
            return result;
        };
        for (const ExternalRoomId id : needsNewCoordinate) {
            const auto idx = lookupTable.at(id);
            auto &coord = input[idx].position;
            output.movedRooms.emplace_back(MovedRoom{id, coord});
            coord = assignUnusedPosition(coord.z);
        }

        numRepairedCoords = needsNewCoordinate.size();
    };

    struct NODISCARD ExitRepairStats final
    {
        size_t addedDoorFlag = 0;
        size_t addedExitFlag = 0;
        size_t removedDoorFlag = 0;
        size_t removedExitFlag = 0;

        size_t removedDoorName = 0;
        size_t removedDoorFlagsExt = 0;
    };

    ExitRepairStats exitRepairStats;
    auto checkAndRepairFlags = [&output, &exitRepairStats](ExternalRawRoom &r,
                                                           const ExitDirEnum dir) {
        auto &exit = r.exits[dir];
        auto &set = exit.getOutgoingSet();

        auto &fields = exit.fields;
        auto &exitFlags = fields.exitFlags;

        const bool hasExits = !set.empty();
        const bool hasDoorFlags = !fields.doorFlags.empty();
        const bool hasDoorName = !fields.doorName.empty();
        const bool hasActualDoorFlag = exitFlags.contains(ExitFlagEnum::DOOR);

        const bool shouldHaveExitFlag = hasExits;
        const bool shouldHaveDoorFlag = shouldHaveExitFlag
                                        && (hasActualDoorFlag || hasDoorFlags || hasDoorName);

        const bool hasExitFlag = exitFlags.contains(ExitFlagEnum::EXIT);
        if (shouldHaveExitFlag && !hasExitFlag) {
            exitFlags.insert(ExitFlagEnum::EXIT);
            exitRepairStats.addedExitFlag += 1;
        } else if (!shouldHaveExitFlag && hasExitFlag) {
            exitFlags.remove(ExitFlagEnum::EXIT);
            exitRepairStats.removedExitFlag += 1;
        }

        if (shouldHaveDoorFlag && !hasActualDoorFlag) {
            exitFlags.insert(ExitFlagEnum::DOOR);
            exitRepairStats.addedDoorFlag += 1;
        }
        if (!shouldHaveDoorFlag) {
            if (hasActualDoorFlag) {
                exitFlags.remove(ExitFlagEnum::DOOR);
                exitRepairStats.removedDoorFlag += 1;
            }

            if (hasDoorName) {
                const auto &doorName = fields.doorName.getStdStringViewUtf8();
                if (!doorName.empty() && (doorName.size() > 1 || doorName.back() != C_NEWLINE)) {
                    output.removedDoors.emplace_back(
                        RemovedDoorName{r.getId(), dir, fields.doorName});
                }
                fields.doorName = {};
                exitRepairStats.removedDoorName += 1;
            }

            if (hasDoorFlags) {
                fields.doorFlags = {};
                exitRepairStats.removedDoorFlagsExt += 1;
            }
        }
    };

    struct NODISCARD ConnectionRepairStats final
    {
        size_t missingIn = 0;
        size_t missingOut = 0;
        size_t removedIn = 0;
        size_t removedOut = 0;
    };

    ConnectionRepairStats connectionRepairStats;
    {
        counter.setCurrentTask(ProgressMsg{"checking exits and flags"});
        counter.reset();
        counter.increaseTotalStepsBy(input.size());
        for (auto &r : input) {
            for (const ExitDirEnum dir : ALL_EXITS7) {
                checkAndRepairExits(r,
                                    dir,
                                    InOutEnum::Out,
                                    connectionRepairStats.missingIn,
                                    connectionRepairStats.removedOut);
                checkAndRepairExits(r,
                                    dir,
                                    InOutEnum::In,
                                    connectionRepairStats.missingOut,
                                    connectionRepairStats.removedIn);
                checkAndRepairFlags(r, dir);
            }
            counter.step();
        }

        if (!hasUniqueCoords()) {
            checkAndRepairCoords();
            if (!hasUniqueCoords()) {
                std::abort();
            }
        }
    }

    {
        auto &&os = MMLOG();
        {
            auto printIfNonzero = [&os](size_t count, std::string_view how, std::string_view what) {
                if (count == 0) {
                    return;
                }

                os << "[sanitize] " << how << " " << count << " " << what << " connection"
                   << ((count == 1) ? "" : "s") << ".\n";
            };

            printIfNonzero(connectionRepairStats.missingIn, "Added", "missing IN");
            printIfNonzero(connectionRepairStats.missingOut, "Added", "missing OUT");
            printIfNonzero(connectionRepairStats.removedIn, "Removed", "invalid IN");
            printIfNonzero(connectionRepairStats.removedOut, "Removed", "invalid OUT");
        }
        {
            auto printIfNonzero = [&os](size_t count, std::string_view how, std::string_view what) {
                if (count == 0) {
                    return;
                }
                os << "[sanitize] " << how << " " << count << " " << what
                   << ((count == 1) ? "" : "s") << ".\n";
            };
            printIfNonzero(exitRepairStats.addedExitFlag, "Added", "missing EXIT flag");
            printIfNonzero(exitRepairStats.addedDoorFlag, "Added", "missing DOOR flag");
            printIfNonzero(exitRepairStats.removedExitFlag, "Removed", "invalid EXIT flag");
            printIfNonzero(exitRepairStats.removedDoorFlag, "Removed", "invalid DOOR flag");
            printIfNonzero(exitRepairStats.removedDoorFlagsExt,
                           "Removed",
                           "invalid extended door flag");
            printIfNonzero(exitRepairStats.removedDoorName, "Removed", "invalid door name");
        }

        if (numRepairedCoords != 0) {
            if (numRepairedCoords == 1) {
                os << "[sanitize] WARNING: Altered the position of ";
                os << "1 room";
                os << " with a duplicate coordinate.\n";
            } else {
                os << "[sanitize] WARNING: Altered the positions of ";
                os << numRepairedCoords << " rooms";
                os << " with duplicate coordinates.\n";
            }
        }

        if (!output.removedDoors.empty()) {
            const auto count = output.removedDoors.size();
            os << "[sanitize] Will attempt to add " << count << " removed door name"
               << ((count == 1) ? "" : "s") << " to room notes after loading.\n";
        }

        if (!output.movedRooms.empty()) {
            const auto count = output.movedRooms.size();
            os << "[sanitize] Will attempt to move " << count;
            os << " room" << ((count == 1) ? "" : "s");
            os << " back as close as possible to ";
            os << ((count == 1) ? "its" : "their");
            os << " specified ";
            os << "position" << ((count == 1) ? "" : "s");
            os << " after loading.\n";
        }
    }

    return output;
}

static ChangeList buildChangelist(ProgressCounter &pc,
                                  const Map &base,
                                  const SanitizerChanges &sanitizeOutput)
{
    std::map<RoomId, RoomNote> notes;
    pc.increaseTotalStepsBy(sanitizeOutput.removedDoors.size());
    for (const auto &x : sanitizeOutput.removedDoors) {
        if (x.name.empty()) {
            assert(false);
            continue;
        }

        const auto room = base.findRoomHandle(x.room);
        if (!room) {
            assert(false);
            continue;
        }

        std::string_view name = x.name.getStdStringViewUtf8();
        trim_newline_inplace(name);

        if (name.empty()) {
            assert(false);
            continue;
        }

        RoomNote before = [&notes, &room]() -> RoomNote {
            if (const auto it = notes.find(room.getId()); it != notes.end()) {
                return it->second;
            }
            return room.getNote();
        }();

        notes[room.getId()] = [&before, dir = x.dir, name]() -> RoomNote {
            std::ostringstream oss;
            const auto &old = before.getStdStringViewUtf8();
            oss << old;
            if (!old.empty() && old.back() != C_NEWLINE) {
                oss << C_NEWLINE;
            }
            oss << "auto-removed door name " << to_string_view(dir) << ": ";
            oss << name;
            oss << C_NEWLINE;
            return RoomNote{std::move(oss).str()};
        }();
        pc.step();
    }

    std::map<RoomId, Coordinate> movedRooms;
    pc.increaseTotalStepsBy(sanitizeOutput.movedRooms.size());
    for (const MovedRoom &x : sanitizeOutput.movedRooms) {
        const auto room = base.findRoomHandle(x.room);
        if (!room) {
            assert(false);
            continue;
        }
        movedRooms[room.getId()] = x.original;
        pc.step();
    }

    ChangeList changes;
    pc.increaseTotalStepsBy(notes.size());
    for (const auto &kv : notes) {
        changes.add(
            room_change_types::ModifyRoomFlags{kv.first, kv.second, FlagModifyModeEnum::ASSIGN});
        pc.step();
    }

    pc.increaseTotalStepsBy(movedRooms.size());
    for (const auto &kv : movedRooms) {
        changes.add(room_change_types::TryMoveCloseTo{kv.first, kv.second});
        pc.step();
    }

    return changes;
}

static Map applySanitizerChanges(ProgressCounter &pc,
                                 const Map &base,
                                 const SanitizerChanges &sanitizeOutput)
{
    const auto changes = buildChangelist(pc, base, sanitizeOutput);
    if (changes.empty()) {
        MMLOG() << "[sanitize] No post-load changes necessary.";
        MMLOG() << "[sanitize] Success.";
        return base;
    }

    const auto count = changes.getChanges().size();
    MMLOG() << "[sanitize] Applying " << count << " change" << ((count == 1) ? "" : "s") << "...";

    MapApplyResult result = base.apply(pc, changes);
    MMLOG() << "[sanitize] Success.";
    static_cast<void>(result.roomUpdateFlags); /* ignored */
    return std::move(result.map);
}

static void sortIfNecessary(ProgressCounter &counter, std::vector<ExternalRawRoom> &input)
{
    DECL_TIMER(t1, "verify-rooms-sorted");
    static auto in_order = [](const ExternalRawRoom &a, const ExternalRawRoom &b) -> bool {
        return a.id < b.id;
    };

    counter.reset();
    counter.setCurrentTask(ProgressMsg{"testing if rooms are sorted"});
    counter.increaseTotalStepsBy(1);
    const bool sorted = std::is_sorted(input.begin(), input.end(), in_order);
    counter.step();
    if (sorted) {
        return;
    }

    DECL_TIMER(t2, "sort-rooms");
    counter.reset();
    counter.setCurrentTask(ProgressMsg{"sorting rooms"});
    counter.increaseTotalStepsBy(1);
    std::sort(input.begin(), input.end(), in_order);
    counter.step(1);
}

MapPair WorldBuilder::build(ProgressCounter &pc, std::vector<ExternalRawRoom> input)
{
    if (input.empty()) {
        return MapPair{};
    }

    DECL_TIMER(t, "build-map");
    sortIfNecessary(pc, input);
    const auto sanitizerChanges = sanitize(pc, input);
    const Map base{World::init(pc, input)};
    return MapPair{base, applySanitizerChanges(pc, base, sanitizerChanges)};
}

MapPair WorldBuilder::build() &&
{
    return build(m_counter, std::exchange(m_rooms, {}));
}

MapPair WorldBuilder::buildFrom(ProgressCounter &counter, std::vector<ExternalRawRoom> rooms)
{
    return WorldBuilder(counter, std::move(rooms)).build();
}
