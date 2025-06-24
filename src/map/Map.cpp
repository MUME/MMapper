// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Map.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/LineUtils.h"
#include "../global/PrintUtils.h"
#include "../global/Timer.h"
#include "../global/logging.h"
#include "../global/parserutils.h"
#include "../global/progresscounter.h"
#include "../global/tests.h"
#include "../global/thread_utils.h"
#include "Changes.h"
#include "Diff.h"
#include "ParseTree.h"
#include "World.h"
#include "WorldBuilder.h"
#include "enums.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <tuple>
#include <vector>

using namespace char_consts;

static constexpr auto green = getRawAnsi(AnsiColor16Enum::green);
static constexpr auto yellow = getRawAnsi(AnsiColor16Enum::yellow);

Map::Map()
    : m_world(std::make_shared<const World>())
{}

Map::Map(World world)
    : m_world(std::make_shared<const World>(std::move(world)))
{}

Map::Map(std::shared_ptr<const World> world)
    : m_world(std::move(world))
{
    std::ignore = deref(m_world);
}

const InfomarkDb &Map::getInfomarkDb() const
{
    return getWorld().getInfomarkDb();
}

size_t Map::getRoomsCount() const
{
    return getWorld().getRoomSet().size();
}

size_t Map::getMarksCount() const
{
    return getWorld().getInfomarkDb().getIdSet().size();
}

bool Map::empty() const
{
    return getRoomsCount() == 0 && getMarksCount() == 0;
}

std::optional<Bounds> Map::getBounds() const
{
    return getWorld().getBounds();
}

const RoomIdSet &Map::getRooms() const
{
    return getWorld().getRoomSet();
}

RoomIdSet Map::findAllRooms(const ParseEvent &parseEvent) const
{
    const Map &map = *this;
    if (map.empty()) {
        return RoomIdSet{};
    }
    const auto &tree = getWorld().getParseTree();
    return ::getRooms(map, tree, parseEvent);
}

NODISCARD const RawRoom *Map::find_room_ptr(const RoomId id) const
{
    return getWorld().getRoom(id);
}

RoomHandle Map::findRoomHandle(const RoomId id) const
{
    if (id != INVALID_ROOMID) {
        if (const RawRoom *const ptr = find_room_ptr(id)) {
            return RoomHandle{Badge<Map>{}, *this, ptr};
        }
    }
    return RoomHandle{};
}

RoomHandle Map::findRoomHandle(const ExternalRoomId ext) const
{
    if (const RoomId id = getWorld().convertToInternal(ext); id != INVALID_ROOMID) {
        return findRoomHandle(id);
    }
    return RoomHandle{};
}

RoomHandle Map::findRoomHandle(const ServerRoomId serverId) const
{
    if (serverId != INVALID_SERVER_ROOMID) {
        if (auto roomId = getWorld().lookup(serverId)) {
            return findRoomHandle(*roomId);
        }
    }
    return RoomHandle{};
}

RoomHandle Map::findRoomHandle(const Coordinate &coord) const
{
    if (auto optRoom = getWorld().findRoom(coord)) {
        return getRoomHandle(*optRoom);
    }
    return RoomHandle{};
}

RoomHandle Map::getRoomHandle(const RoomId id) const
{
    if (auto h = findRoomHandle(id)) {
        return h;
    }
    throw InvalidMapOperation("RoomId not found");
}

RoomHandle Map::getRoomHandle(const ExternalRoomId id) const
{
    if (auto h = findRoomHandle(id)) {
        return h;
    }
    throw InvalidMapOperation("ExternalRoomId not found");
}

const RawRoom &Map::getRawRoom(const RoomId id) const
{
    if (id != INVALID_ROOMID) {
        if (const RawRoom *const ptr = find_room_ptr(id)) {
            return deref(ptr);
        }
    }
    throw InvalidMapOperation("RoomId not found");
}

std::optional<DoorName> Map::findDoorName(const RoomId id, const ExitDirEnum dir) const
{
    const World &world = getWorld();
    if (world.hasRoom(id)) {
        if (world.getExitFlags(id, dir).isDoor()) {
            return world.getExitDoorName(id, dir);
        }
    }
    return std::nullopt;
}

static void reportDetectedChanges(std::ostream &os, const WorldComparisonStats &stats)
{
    os << "[update] The following changes were detected:\n";

    do {
#define SHOW(x) show(#x, stats.x)
        auto show = [&os](std::string_view k, bool value) {
            if (value) {
                os << "[update]   ... " << k << ": YES.\n";
            }
        };

        SHOW(anyRoomsRemoved);
        SHOW(anyRoomsAdded);

        // short-circuit, because all of the following will be true
        if (stats.anyRoomsRemoved || stats.anyRoomsAdded) {
            break;
        }

        SHOW(spatialDbChanged);
        // SHOW(parseTreeChanged);
        SHOW(serverIdsChanged);

        SHOW(hasMeshDifferences);

#undef SHOW
    } while (false);
    os << "[update] End of changes detected.\n";
}

NODISCARD static RoomUpdateFlags reportNeededUpdates(std::ostream &os,
                                                     const WorldComparisonStats &stats)
{
    // REVISIT: actually it doesn't matter if Align or Portable changed,
    // but there's no way to quickly test those individually on this branch.

    const bool needRoomMeshUpdate = stats.hasMeshDifferences;
    const bool boundsChanged = stats.boundsChanged;
    const bool marksChanged = stats.anyInfomarksChanged;

    os << "[update] Bounds changed: " << (boundsChanged ? "YES" : "NO") << ".\n";
    os << "[update] Marks changed: " << (marksChanged ? "YES" : "NO") << ".\n";
    os << "[update] Needs any mesh updates: " << (needRoomMeshUpdate ? "YES" : "NO") << ".\n";

    RoomUpdateFlags result;
    if (boundsChanged) {
        result.insert(RoomUpdateEnum::BoundsChanged);
    }
    if (marksChanged) {
        result.insert(RoomUpdateEnum::MarksChanged);
    }
    if (needRoomMeshUpdate) {
        result.insert(RoomUpdateEnum::RoomMeshNeedsUpdate);
    }
    return result;
}

template<typename Callback>
NODISCARD static MapApplyResult update(const std::shared_ptr<const World> &input,
                                       ProgressCounter &pc,
                                       Callback &&callback)
{
    static const bool verbose_debugging = IS_DEBUG_BUILD;

    using Clock = std::chrono::steady_clock;
    // verify that callback signature is `void(World&)`.
    static_assert(std::is_invocable_r_v<void, Callback, ProgressCounter &, World &>);

    const auto t0 = Clock::now();

    const World &base = deref(input);
    World modified = base.copy();

    const auto t1 = Clock::now();
    callback(pc, modified);
    const auto t2 = Clock::now();
    bool equal = base == modified;
    const auto t3 = Clock::now();

    RoomUpdateFlags neededRoomUpdates;
    std::ostringstream info_os;

    if (equal) {
        if (verbose_debugging) {
            info_os << "[update] No change detected, but we'll still return the modified world object just in case.\n";
        }
    } else {
        try {
            const auto stats = World::getComparisonStats(base, modified);
            reportDetectedChanges(info_os, stats);
            neededRoomUpdates = reportNeededUpdates(info_os, stats);
        } catch (const std::exception &ex) {
            info_os << "[update] Changes detected, but an exception occurred while comparing: "
                    << ex.what() << ".\n";
        }
    }
    const auto t4 = Clock::now();

    MMLOG() << info_os.str(); // not included in the timing

    if (verbose_debugging) {
        std::ostringstream debug_os;
        auto report = [&debug_os](std::string_view what, Clock::time_point a, Clock::time_point b) {
            auto ms = static_cast<double>(static_cast<std::chrono::nanoseconds>(b - a).count())
                      * 1e-6;
            if (verbose_debugging) {
                debug_os << "[TIMER] [update] " << what << ": " << ms << " ms\n";
            }
        };
        report("part0. modified = base.copy()", t0, t1);
        report("part1. callback(modified)", t1, t2);
        report("part2. base == modified", t2, t3);
        report("part3. stats + report changes", t3, t4);
        report("part0 + part1 (required)", t0, t2);
        report("part2 + part3 (deferrable)", t2, t4);
        report("overall", t0, t4);
        MMLOG_DEBUG() << std::move(debug_os).str();
    }

    // only modify input if the callback succeeds
    return MapApplyResult{Map{std::move(modified)}, neededRoomUpdates};
}

MapApplyResult Map::applySingleChange(ProgressCounter &pc, const Change &change) const
{
    {
        MMLOG() << "[map] Applying 1 change...\n";
    }
    return update(m_world, pc, [&change](ProgressCounter &pc2, World &w) {
        w.applyOne(pc2, change);
    });
}

Map Map::filterBaseMap(ProgressCounter &pc) const
{
    MapApplyResult mar = applySingleChange(pc, Change{world_change_types::GenerateBaseMap{}});
    return mar.map;
}

MapApplyResult Map::apply(ProgressCounter &pc, const std::vector<Change> &changes) const
{
    if (changes.empty()) {
        throw InvalidMapOperation("Changes are empty");
    }

    {
        const auto count = changes.size();
        MMLOG() << "[map] Applying " << count << " change" << ((count == 1) ? "" : "s") << "...\n";
    }
    return update(m_world, pc, [&changes](ProgressCounter &pc2, World &w) {
        w.applyAll(pc2, changes);
    });
}

MapApplyResult Map::apply(ProgressCounter &pc, const ChangeList &changeList) const
{
    return apply(pc, changeList.getChanges());
}

MapPair Map::fromRooms(ProgressCounter &counter,
                       std::vector<ExternalRawRoom> rooms,
                       std::vector<InfoMarkFields> marks)
{
    return WorldBuilder::buildFrom(counter, std::exchange(rooms, {}), std::exchange(marks, {}));
}

void Map::printStats(ProgressCounter &pc, AnsiOstream &aos) const
{
    getWorld().printStats(pc, aos);
}

void Map::checkConsistency(ProgressCounter &counter) const
{
    getWorld().checkConsistency(counter);
}

void Map::printMulti(ProgressCounter &pc, AnsiOstream &os) const
{
    const World &w = getWorld();

    std::set<ExternalRoomId> rooms;
    pc.setNewTask(ProgressMsg{"phase 1: scanning rooms"}, getRoomsCount());
    for (const RoomId here : getRooms()) {
        const auto &room = deref(w.getRoom(here));
        const auto hereExternal = w.convertToExternal(here);
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
            const auto &ex = room.getExit(dir);
            if (ex.exitIsRandom()) {
                continue;
            }
            rooms.insert(hereExternal);
            break;
        }
        pc.step();
    }

    pc.setNewTask(ProgressMsg{"phase 2: processing rooms"}, rooms.size());
    for (const ExternalRoomId hereExternal : rooms) {
        const auto &room = getRoomHandle(hereExternal);
        const RoomId self = room.getId();
        for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
            const auto rev = opposite(dir);
            const auto &ex = room.getExit(dir);
            if (ex.exitIsRandom()) {
                continue;
            }

            if (ex.getOutgoingSet().size() > 1) {
                os << hereExternal.value() << " (";
                os.writeQuotedWithColor(green, yellow, room.getName().getStdStringViewUtf8());
                os << ")";
                {
                    const auto &pos = room.getPosition();
                    os << " at Coordinate(" << pos.x << ", " << pos.y << ", " << pos.z << ")";
                }
                os << " connects " << to_string_view(dir) << " to...\n";
                for (const RoomId real_to : ex.getOutgoingSet()) {
                    const ExternalRoomId to = w.convertToExternal(real_to);
                    os << " ...";
                    if (const auto other = this->findRoomHandle(to)) {
                        bool twoWay = false;
                        {
                            const auto &otherExit = other.getExit(rev);
                            if (otherExit.containsOut(self)) {
                                twoWay = true;
                            }
                        }

                        const bool looping = hereExternal == to;
                        const bool adj = !looping
                                         && room.getPosition() + exitDir(dir)
                                                == other.getPosition();

                        os << (twoWay ? "two" : "one") << "-way ";
                        if (looping) {
                            os << "looping";
                        } else if (adj) {
                            os << "adjacent";
                        } else {
                            os << "non-adjacent";
                        }
                        os << " " << to_string_view(dir) << " to ";
                        if (looping) {
                            os << "itself";
                        } else {
                            os << to.value();
                            os << " (";
                            os.writeQuotedWithColor(green,
                                                    yellow,
                                                    other.getName().getStdStringViewUtf8());
                            os << ")";

                            if (!adj) {
                                const auto &pos = other.getPosition();
                                os << " at Coordinate(" << pos.x << ", " << pos.y << ", " << pos.z
                                   << ")";
                            }
                        }
                    } else {
                        os << to.value();
                    }
                    os << "\n";
                }
                os << "\n";
            }
        }
        pc.step();
    }
}

void Map::printUnknown(ProgressCounter &pc, AnsiOstream &os) const
{
    std::set<ExternalRoomId> set;
    pc.setNewTask(ProgressMsg{"scanning rooms"}, getRoomsCount());
    for (const RoomId id : getRooms()) {
        const auto &room = getRoomHandle(id);
        if (!room.getExit(ExitDirEnum::UNKNOWN).outIsEmpty()
            || !room.getExit(ExitDirEnum::UNKNOWN).inIsEmpty()) {
            set.insert(getExternalRoomId(id));
        }
        pc.step();
    }

    const auto unknownStr = "Unknown";

    if (set.empty()) {
        os << "There are no legacy ";
        os.writeWithColor(green, unknownStr);
        os << " exits.\n ";
        return;
    }

    os << "The following " << set.size() << " room" << ((set.size() == 1) ? "" : "s")
       << " have at least one legacy ";
    os.writeWithColor(green, unknownStr);
    os << " entrance or exit:\n";

    pc.setNewTask(ProgressMsg{"reporting results"}, set.size());
    for (const ExternalRoomId extId : set) {
        const auto &room = getRoomHandle(extId);
        os << extId.value() << ": ";
        os.writeQuotedWithColor(green, yellow, room.getName().getStdStringViewUtf8());
        {
            const auto &pos = room.getPosition();
            os << " at Coordinate(" << pos.x << ", " << pos.y << ", " << pos.z << ")";
        }
        os << "\n";
        pc.step();
    }
}

bool Map::operator==(const Map &other) const
{
    return m_world == other.m_world || deref(m_world) == deref(other.m_world);
}

void Map::diff(ProgressCounter &pc, AnsiOstream &os, const Map &a, const Map &b)
{
    struct NoCopyDefaultMove
    {
        NoCopyDefaultMove() = default;
        MAYBE_UNUSED ~NoCopyDefaultMove() = default;
        NoCopyDefaultMove(const NoCopyDefaultMove &) = delete;
        NoCopyDefaultMove &operator=(const NoCopyDefaultMove &) = delete;
        MAYBE_UNUSED NoCopyDefaultMove(NoCopyDefaultMove &&) = default;
        MAYBE_UNUSED NoCopyDefaultMove &operator=(NoCopyDefaultMove &&) = default;
    };

    struct NODISCARD Sets final : public NoCopyDefaultMove
    {
        std::set<ExternalRoomId> removedSet;
        std::set<ExternalRoomId> addedSet;
        std::set<ExternalRoomId> commonSet;

        static void add_all(std::set<ExternalRoomId> &to, const std::set<ExternalRoomId> &from)
        {
            to.insert(from.begin(), from.end());
        }

        Sets &operator+=(const Sets &other)
        {
            add_all(removedSet, other.removedSet);
            add_all(addedSet, other.addedSet);
            add_all(commonSet, other.commonSet);
            return *this;
        }
    };

    DECL_TIMER(t, "Map::diff (parallel)");

    const World &aWorld = a.getWorld();
    const World &bWorld = b.getWorld();

    Sets sets;
    auto merge_sets_tls = [&sets](auto &tls) {
        for (auto &tl : tls) {
            sets += tl;
        }
    };

    pc.setNewTask(ProgressMsg{"scanning old rooms"}, a.getRoomsCount());
    thread_utils::parallel_for_each_tl<Sets>(
        a.getRooms(),
        pc,
        [&aWorld, &b](auto &tl, const RoomId oldRoom) {
            const ExternalRoomId extId = aWorld.convertToExternal(oldRoom);
            if (b.findRoomHandle(extId)) {
                tl.commonSet.insert(extId);
            } else {
                tl.removedSet.insert(extId);
            }
        },
        merge_sets_tls);

    pc.setNewTask(ProgressMsg{"scanning new rooms"}, b.getRoomsCount());
    thread_utils::parallel_for_each_tl<Sets>(
        b.getRooms(),
        pc,
        [&bWorld, &a](auto &tl, const RoomId newRoom) {
            const ExternalRoomId extId = bWorld.convertToExternal(newRoom);
            if (!a.findRoomHandle(extId)) {
                tl.addedSet.insert(extId);
            }
        },
        merge_sets_tls);

    bool hasChange = false;

    struct NODISCARD TlReporter : NoCopyDefaultMove
    {
    public:
        struct [[nodiscard]] Detail final
        {
            std::ostringstream os;
            AnsiOstream aos{os};
            OstreamDiffReporter odr{aos};
        };

    private:
        std::unique_ptr<Detail> m_detail = std::make_unique<Detail>();

    public:
        NODISCARD Detail &get() { return deref(m_detail); }
        NODISCARD std::string finish()
        {
            auto result = std::move(deref(m_detail).os).str();
            m_detail.reset();
            return result;
        }
    };

    auto merge_tl_reporters = [&os](auto &tls) {
        for (auto &tl : tls) {
            os.writeWithEmbeddedAnsi(tl.finish());
        }
    };

    {
        if (!sets.removedSet.empty()) {
            hasChange = true;
            os << "Removed rooms:\n";
            os << "\n";
            pc.setNewTask(ProgressMsg{"reporting removed rooms"}, sets.removedSet.size());
            thread_utils::parallel_for_each_tl<TlReporter>(
                sets.removedSet,
                pc,
                [&a](auto &tl, const ExternalRoomId extId) {
                    const auto &oldRoom = a.getRoomHandle(extId);
                    auto &detail = tl.get();
                    detail.os << "Removed room " << extId.value() << ":\n";
                    detail.odr.removed(oldRoom);
                },
                merge_tl_reporters);
        }

        if (!sets.addedSet.empty()) {
            if (hasChange) {
                os << "\n";
            }
            hasChange = true;
            os << "Added rooms:\n";
            os << "\n";
            pc.setNewTask(ProgressMsg{"reporting added rooms"}, sets.addedSet.size());
            thread_utils::parallel_for_each_tl<TlReporter>(
                sets.addedSet,
                pc,
                [&b](auto &tl, const ExternalRoomId extId) {
                    const auto &oldRoom = b.getRoomHandle(extId);
                    auto &detail = tl.get();
                    detail.os << "Added room " << extId.value() << ":\n";
                    detail.odr.added(oldRoom);
                },
                merge_tl_reporters);
        }
    }

    {
        struct TlReporter2 final : public TlReporter
        {
            bool printedAny = false;
        };

        bool printed_first_change = false;
        auto merge_tlreporter2 = [&os, &hasChange, &printed_first_change](auto &tls) {
            for (TlReporter2 &tl : tls) {
                if (!tl.printedAny) {
                    continue;
                }

                if (!printed_first_change) {
                    if (hasChange) {
                        os << "\n";
                    }
                    os << "Changes to existing rooms:\n";
                    printed_first_change = true;
                }

                hasChange = true;
                os.writeWithEmbeddedAnsi(tl.finish());
            }
        };

        pc.setNewTask(ProgressMsg{"scanning common rooms"}, sets.commonSet.size());
        thread_utils::parallel_for_each_tl<TlReporter2>(
            sets.commonSet,
            pc,
            [&a, &b](TlReporter2 &tl, const ExternalRoomId extId) {
                const auto &oldRoom = a.getRoomHandle(extId);
                const auto &newRoom = b.getRoomHandle(extId);
                std::ostringstream oss;
                {
                    AnsiOstream aos{oss};
                    OstreamDiffReporter odr{aos};
                    compare(odr, oldRoom, newRoom);
                }
                auto str = std::move(oss).str();
                if (str.empty()) {
                    return;
                }

                tl.printedAny = true;
                auto &detail = tl.get();
                detail.os << "\n";
                detail.os << "Changes to room " << extId.value() << ":\n";
                detail.aos.writeWithEmbeddedAnsi(str);
            },
            merge_tlreporter2);
    }

    if (!hasChange) {
        os << "None.\n";
    } else {
        os << "\n";
        os << "End of changes.\n";
    }
}

static void print_coordinate(AnsiOstream &os, const RawAnsi &ansi, const Coordinate &where)
{
    os << "(";
    os.writeWithColor(ansi, where.x);
    os << ", ";
    os.writeWithColor(ansi, where.y);
    os << ", ";
    os.writeWithColor(ansi, where.z);
    os << ")";
}

void Map::statRoom(AnsiOstream &os, RoomId id) const
{
    const auto &room = getRoomHandle(id);
    const auto &pos = room.getPosition();

    const auto ansi_cyan = getRawAnsi(AnsiColor16Enum::cyan);
    const auto ansi_green = getRawAnsi(AnsiColor16Enum::green);
    const auto ansi_yellow = getRawAnsi(AnsiColor16Enum::yellow);
    const auto ansi_red = getRawAnsi(AnsiColor16Enum::red);

    auto kv = [&ansi_green, &os](std::string_view k, std::string_view v) {
        os << k << ": ";
        os.writeWithColor(ansi_green, v);
    };

    auto print_flags = [&ansi_green, &os](const auto flags) {
        if (flags.empty()) {
            os << " (none)";
        } else {
            for (const auto flag : flags) {
                os << " ";
                os.writeWithColor(ansi_green, to_string_view(flag));
            }
        }
    };

    os << "Room ";
    os.writeWithColor(ansi_green, getExternalRoomId(id).value());
    os << " (internal ID: ";
    os.writeWithColor(ansi_green, room.getId().value());
    os << "), Server ID: ";
    if (auto sid = room.getServerId(); sid != INVALID_SERVER_ROOMID) {
        os.writeWithColor(ansi_green, room.getServerId().value());
    } else {
        os.writeWithColor(ansi_yellow, "undefined");
    }
    os << ", Coordinate";
    print_coordinate(os, ansi_green, pos);
    os << "\n";

    os << "Area: ";
    {
        const RoomArea &areaName = room.getArea();
        const auto numInArea = [this, &areaName]() {
            if (auto opt = countRoomsWithArea(areaName)) {
                return *opt;
            }
            // Other callers might be willing to tolerate failure,
            // but it's a hard map consistency error here if the area doesn't exist.
            throw std::runtime_error("invalid area");
        }();
        const auto relativeSize = [this, &numInArea]() {
            // std::fmt should make this a lot easier when that's available.
            char buf[32];
            const auto pct = 100.0 * static_cast<double>(numInArea)
                             / static_cast<double>(getRoomsCount());
            std::snprintf(buf, sizeof(buf), "%.1f", pct);
            return std::string{buf};
        }();
        if (!areaName.empty()) {
            os.writeQuotedWithColor(ansi_green, ansi_yellow, areaName.getStdStringViewUtf8());
        } else {
            os.writeWithColor(ansi_yellow, "undefined");
        }
        os << " (";
        os << "relative size: ";
        os.writeWithColor(ansi_green, relativeSize);
        os.writeWithColor(ansi_yellow, "%");
        os << ", rooms: ";
        os.writeWithColor(ansi_green, numInArea);
        os << ")";
    }
    os << "\n";

    os << "Name: ";
    {
        const RoomName &name = room.getName();
        os.writeQuotedWithColor(ansi_green, ansi_yellow, name.getStdStringViewUtf8());
        if (!name.empty()) {
            // TODO: report these stats within the current area
            // and then _maybe_ also report the global values
            // (right now it only shows the global values).
            os << " [";

            const auto &desc = room.getDescription();
            const auto nameCount = countRoomsWithName(name);
            const size_t descCount = countRoomsWithDesc(desc);

            if (nameCount == 1) {
                os.writeWithColor(ansi_yellow, "unique name");
                if (descCount == 1) {
                    os << ", and ";
                    os.writeWithColor(ansi_yellow, "unique desc");
                }
            } else {
                os << "name collisions: ";
                os.writeWithColor(ansi_green, nameCount);

                if (descCount == 1) {
                    os << ", but ";
                    os.writeWithColor(ansi_yellow, "unique desc");
                } else {
                    const size_t nameDescCount = countRoomsWithNameDesc(name, desc);
                    if (nameDescCount == 1) {
                        os << ", but ";
                        os.writeWithColor(ansi_yellow, "unique name+desc");
                    } else {
                        if (nameCount == nameDescCount) {
                            os << "; all with same name/desc";
                        } else {
                            os << "; name/desc collisions: ";
                            os.writeWithColor(ansi_green, nameDescCount);
                        }
                    }
                }
            }
            os << "]";
        }
    }
    os << "\n";

    auto getStatus = [&room]() -> std::string_view {
        return room.isTemporary() ? "TEMPORARY" : "PERMANENT";
    };

    kv("Status", getStatus());

    os << ", ";
    kv("Sector", to_string_view(room.getTerrainType()));
    os << "\n";

    kv("Align", to_string_view(room.getAlignType()));
    os << ", ";
    kv("Light", to_string_view(room.getLightType()));
    os << ", ";
    kv("Portable", to_string_view(room.getPortableType()));
    os << ", ";
    kv("Rideable", to_string_view(room.getRidableType()));
    os << ", ";
    kv("Sundeath", to_string_view(room.getSundeathType()));
    os << "\n";

    os << "Mob Flags:";
    print_flags(room.getMobFlags());
    os << "\n";

    os << "Load Flags:";
    print_flags(room.getLoadFlags());
    os << "\n";

    auto print_quoted_lines = [&os, &ansi_green, &ansi_yellow](std::string_view k,
                                                               std::string_view v) {
        os << "\n";
        os << k << ":\n";

        auto print = [&os, &ansi_green, &ansi_yellow](std::string_view sv) {
            os.writeQuotedWithColor(ansi_green, ansi_yellow, sv);
            os << "\n";
        };

        if (v.empty()) {
            print("");
        } else {
            foreachLine(v, print);
        }
    };

    const auto &desc = room.getDescription();
    print_quoted_lines("Description", desc.getStdStringViewUtf8());
    print_quoted_lines("Contents", room.getContents().getStdStringViewUtf8());
    print_quoted_lines("Note", room.getNote().getStdStringViewUtf8());

    const World &world = getWorld();
    auto print_room = [&os, &pos, &world, &ansi_green, &ansi_yellow, &ansi_red](const InOutEnum mode,
                                                                                const RawRoom &other,
                                                                                const bool adj,
                                                                                const bool loop,
                                                                                const bool twoWay) {
        os << "  ";
        if (!twoWay) {
            os.writeWithColor((mode == InOutEnum::Out) ? ansi_yellow : ansi_red,
                              (mode == InOutEnum::Out) ? "OUT" : "IN");
            os << " ";
        }

        if (adj) {
            os << "adjacent";
        } else if (loop) {
            os.writeWithColor(ansi_yellow, "looping");
        } else {
            os.writeWithColor(ansi_red, "distant");
        }

        os << " ";
        if (twoWay) {
            os << "two-way";
        } else {
            os.writeWithColor(ansi_red, "one-way");
        }

        os << " ";
        os << ((mode == InOutEnum::Out) ? "to" : "from");

        os << " ";

        if (loop) {
            os << "itself";
        } else {
            os.writeWithColor(ansi_green, world.convertToExternal(other.getId()).value());
            os << " (";
            os.writeQuotedWithColor(ansi_green, ansi_yellow, other.getName().getStdStringViewUtf8());
            os << ")";
            if (!adj) {
                os << " at Coordinate";
                const auto &otherPos = other.getPosition();
                print_coordinate(os, ansi_green, otherPos);
                os << "; Delta";
                const auto delta = otherPos - pos;
                print_coordinate(os, ansi_green, delta);
            }
        }
        os << "\n";
    };

    // NOTE: This uses initializer-list to get the desired order.
    os << "\n";
    os << "Connections:\n";
    for (const ExitDirEnum dir : {ExitDirEnum::NORTH,
                                  ExitDirEnum::EAST,
                                  ExitDirEnum::SOUTH,
                                  ExitDirEnum::WEST,
                                  ExitDirEnum::UP,
                                  ExitDirEnum::DOWN,
                                  ExitDirEnum::UNKNOWN}) {
        const RawExit &ex = room.getExit(dir);
        const bool isUnknown = dir == ExitDirEnum::UNKNOWN;

        if (!ex.exitIsExit() && ex.outIsEmpty() && ex.inIsEmpty()) {
            continue;
        }

        os << "\n";
        os.writeWithColor(ansi_cyan, to_string_view(dir));
        os << ":";

        if (ex.exitIsExit()) {
            os << "\n  exit flags:";
            print_flags(ex.getExitFlags());
        }

        if (ex.exitIsDoor()) {
            os << "\n  door flags:";
            print_flags(ex.getDoorFlags());
            os << "\n  door name: ";
            os.writeQuotedWithColor(ansi_green,
                                    ansi_yellow,
                                    ex.getDoorName().getStdStringViewUtf8());
        }

        os << "\n";

        const auto rev = opposite(dir);

        if (!ex.outIsEmpty()) {
            for (const RoomId to_id : ex.getOutgoingSet()) {
                const RawRoom &to = deref(world.getRoom(to_id));
                const bool twoWay = to.getExit(rev).containsOut(id);
                const bool adj = !isUnknown && pos + exitDir(dir) == to.getPosition();
                const bool loop = id == to_id;

                print_room(InOutEnum::Out, to, adj, loop, twoWay);
            }
        }

        if (!ex.inIsEmpty()) {
            for (const RoomId from_id : ex.getIncomingSet()) {
                const RawRoom &from = deref(world.getRoom(from_id));
                const bool twoWay = from.getExit(rev).containsIn(id);
                const bool loop = id == from_id;
                if (twoWay /* || loop*/) {
                    continue; // already shown the normal way
                }

                const bool adj = !isUnknown && pos + exitDir(dir) == from.getPosition();
                print_room(InOutEnum::In, from, adj, loop, twoWay);
            }
        }
    }
    // os << "\n";
}

std::optional<size_t> Map::countRoomsWithArea(const RoomArea &areaName) const
{
    const auto &world = getWorld();
    if (const RoomIdSet *const area = world.findAreaRoomSet(areaName)) {
        return area->size();
    }
    return std::nullopt;
}

size_t Map::countRoomsWithName(const RoomName &name) const
{
    const auto &world = getWorld();
    const auto &parseTree = world.getParseTree();
    if (const auto *const pSet = parseTree.name_only.find(name)) {
        return pSet->size();
    }
    return 0;
}

size_t Map::countRoomsWithDesc(const RoomDesc &desc) const
{
    const auto &world = getWorld();
    const auto &parseTree = world.getParseTree();
    if (const auto *const pSet = parseTree.desc_only.find(desc)) {
        return pSet->size();
    }
    return 0;
}

size_t Map::countRoomsWithNameDesc(const RoomName &name, const RoomDesc &desc) const
{
    const auto &world = getWorld();
    const auto &parseTree = world.getParseTree();
    const NameDesc nameDesc{name, desc};
    if (const auto *const pSet = parseTree.name_desc.find(nameDesc)) {
        return pSet->size();
    }
    return 0;
}

std::optional<RoomId> Map::findUniqueName(const RoomName &name) const
{
    const auto &world = getWorld();
    const auto &parseTree = world.getParseTree();
    if (const RoomIdSet *const pSet = parseTree.name_only.find(name)) {
        if (pSet->size() == 1) {
            return pSet->first();
        }
    }
    return std::nullopt;
}

std::optional<RoomId> Map::findUniqueDesc(const RoomDesc &desc) const
{
    const auto &world = getWorld();
    const auto &parseTree = world.getParseTree();
    if (const RoomIdSet *const pSet = parseTree.desc_only.find(desc)) {
        if (pSet->size() == 1) {
            return pSet->first();
        }
    }
    return std::nullopt;
}

std::optional<RoomId> Map::findUniqueNameDesc(const RoomName &name, const RoomDesc &desc) const
{
    const auto &world = getWorld();
    const auto &parseTree = world.getParseTree();
    const NameDesc nameDesc{name, desc};
    if (const RoomIdSet *const pSet = parseTree.name_desc.find(nameDesc)) {
        if (pSet->size() == 1) {
            return pSet->first();
        }
    }
    return std::nullopt;
}

bool Map::hasUniqueName(const RoomId id) const
{
    const auto &world = getWorld();
    const auto &name = world.getRoomName(id);
    return findUniqueName(name) == id;
}

bool Map::hasUniqueDesc(const RoomId id) const
{
    const auto &world = getWorld();
    const auto &desc = world.getRoomDescription(id);
    return findUniqueDesc(desc) == id;
}

bool Map::hasUniqueNameDesc(const RoomId id) const
{
    const auto &world = getWorld();
    const auto &name = world.getRoomName(id);
    const auto &desc = world.getRoomDescription(id);
    return findUniqueNameDesc(name, desc) == id;
}

ExternalRoomId Map::getExternalRoomId(RoomId id) const
{
    return getRoomHandle(id).getIdExternal();
}

Map Map::merge(ProgressCounter &pc,
               const Map &currentMap,
               std::vector<ExternalRawRoom> newRooms,
               std::vector<InfoMarkFields> newMarks,
               const Coordinate &mapOffset)
{
    if (newRooms.empty()) {
        throw std::runtime_error("no rooms to merge");
    }

    {
        // modifies the values in-place
        std::map<ExternalRoomId, ExternalRoomId> remap;
        ExternalRoomId nextId = currentMap.getWorld().getNextExternalId();
        auto alloc_remap = [&remap, &nextId](ExternalRoomId roomId) {
            if (remap.find(roomId) != remap.end()) {
                return;
            }
            remap[roomId] = nextId;
            nextId = nextId.next();
        };

        pc.setNewTask(ProgressMsg{"computing new room IDs (part 1)"}, newRooms.size());

        for (const auto &room : newRooms) {
            alloc_remap(room.id);
            pc.step();
        }

        pc.setNewTask(ProgressMsg{"computing new room IDs (part 2)"}, newRooms.size());

        for (const auto &room : newRooms) {
            for (const auto &exit : room.exits) {
                for (const ExternalRoomId to : exit.outgoing) {
                    alloc_remap(to);
                }
                for (const ExternalRoomId from : exit.incoming) {
                    alloc_remap(from);
                }
            }
            pc.step();
        }

        auto updateId_inplace = [&remap](ExternalRoomId &id) { id = remap.at(id); };

        auto updateIds_inplace = [&remap](TinyExternalRoomIdSet &set) {
            TinyExternalRoomIdSet result;
            for (const ExternalRoomId id : set) {
                result.insert(remap.at(id));
            }
            set = result;
        };

        pc.setNewTask(ProgressMsg{"applying new room IDs"}, newRooms.size());

        for (auto &room : newRooms) {
            updateId_inplace(room.id);
            for (auto &exit : room.exits) {
                updateIds_inplace(exit.outgoing);
                updateIds_inplace(exit.incoming);
            }
            pc.step();
        }
    }

    if (!mapOffset.isNull()) {
        // modifies the values in-place
        pc.setNewTask(ProgressMsg{"offsetting new rooms"}, newRooms.size());

        for (auto &room : newRooms) {
            room.position += mapOffset;
            pc.step();
        }
    }

    // TODO: keep the existing raw data, and insert the new map,
    // instead of making a totally new copy.

    {
        pc.setNewTask(ProgressMsg{"creating combined map"},
                      currentMap.getRoomsCount() + newRooms.size() + currentMap.getMarksCount()
                          + newMarks.size());

        std::vector<ExternalRawRoom> rooms;
        rooms.reserve(currentMap.getRoomsCount() + newRooms.size());
        std::vector<InfoMarkFields> marks;
        marks.reserve(currentMap.getMarksCount() + newMarks.size());

        pc.setCurrentTask(ProgressMsg{"creating combined map: old rooms"});
        for (const RoomId id : currentMap.getRooms()) {
            const RoomHandle &room = currentMap.getRoomHandle(id);
            rooms.emplace_back(room.getRawCopyExternal());
            pc.step();
        }

        pc.setCurrentTask(ProgressMsg{"creating combined map: new rooms"});
        for (const auto &room : newRooms) {
            rooms.emplace_back(room);
            pc.step();
        }

        pc.setCurrentTask(ProgressMsg{"creating combined map: old marks"});
        const auto &db = currentMap.getInfomarkDb();
        for (const auto id : db.getIdSet()) {
            marks.emplace_back(db.getRawCopy(id));
            pc.step();
        }

        const auto infomarkOffset = [&mapOffset]() -> Coordinate {
            const auto tmp = mapOffset.to_ivec3() * glm::ivec3{INFOMARK_SCALE, INFOMARK_SCALE, 1};
            return Coordinate{tmp.x, tmp.y, tmp.z};
        }();

        pc.setCurrentTask(ProgressMsg{"creating combined map: new marks"});
        for (auto &im : newMarks) {
            im.offsetBy(infomarkOffset);
            marks.emplace_back(im);
            pc.step();
        }

        pc.setNewTask(ProgressMsg{"loading"}, 1);

        const auto tmp = Map::fromRooms(pc, std::move(rooms), std::move(marks));
        // NOTE: We're ignoring the base, so that means things like
        // "door names converted to notes" won't show up as a separate diff.
        if ((false) && tmp.base != tmp.modified) {
            // should we report a diff?
        }
        return tmp.modified;
    }
}

void Map::foreachChangedRoom(ProgressCounter &pc,
                             const Map &saved,
                             const Map &current,
                             const std::function<void(const RawRoom &room)> &callback)
{
    pc.increaseTotalStepsBy(current.getRoomsCount());
    for (const RoomId id : current.getRooms()) {
        const auto r = current.findRoomHandle(id);
        if (!r) {
            assert(false);
            continue;
        }
        const auto &prev = saved.findRoomHandle(id);
        // older code failed to check incoming/outgoing connection differences here
        if (!prev || r.getRaw() != prev.getRaw()) {
            callback(r.getRaw());
        }
        pc.step();
    }
}

NODISCARD bool Map::wouldAllowRelativeMove(const RoomIdSet &set, const Coordinate &offset) const
{
    return getWorld().wouldAllowRelativeMove(set, offset);
}

void Map::printChange(AnsiOstream &aos, const Change &change) const
{
    getWorld().printChange(aos, change);
}
void Map::printChanges(AnsiOstream &aos,
                       const std::vector<Change> &changes,
                       const std::string_view sep) const
{
    getWorld().printChanges(aos, changes, sep);
}

void Map::printChange(std::ostream &os, const Change &change) const
{
    AnsiOstream aos{os};
    printChange(aos, change);
}
void Map::printChanges(std::ostream &os,
                       const std::vector<Change> &changes,
                       const std::string_view sep) const
{
    AnsiOstream aos{os};
    printChanges(aos, changes, sep);
}

void Map::printChange(mm::AbstractDebugOStream &os, const Change &change) const
{
    std::ostringstream oss;
    printChange(oss, change);
    os.writeUtf8(oss.str());
}
void Map::printChanges(mm::AbstractDebugOStream &os,
                       const std::vector<Change> &changes,
                       std::string_view sep) const
{
    std::ostringstream oss;
    printChanges(oss, changes, sep);
    os.writeUtf8(oss.str());
}

void Map::enableExtraSanityChecks(const bool enable)
{
    World::enableExtraSanityChecks(enable);
}

BasicDiffStats getBasicDiffStats(const Map &baseMap, const Map &modMap)
{
    DECL_TIMER(t, "get map diff stats (parallel)");

    const auto &base = baseMap.getWorld();
    const auto &mod = modMap.getWorld();

    ProgressCounter dummyPc;
    BasicDiffStats result;
    auto merge_tls = [&result](auto &tls) {
        for (auto &tl : tls) {
            result += tl;
        }
    };

    thread_utils::parallel_for_each_tl<BasicDiffStats>(
        base.getRoomSet(),
        dummyPc,
        [&mod](auto &tl, const RoomId id) {
            if (!mod.hasRoom(id)) {
                ++tl.numRoomsRemoved;
            }
        },
        merge_tls);

    thread_utils::parallel_for_each_tl<BasicDiffStats>(
        mod.getRoomSet(),
        dummyPc,
        [&base, &mod](auto &tl, const RoomId id) {
            if (!base.hasRoom(id)) {
                ++tl.numRoomsAdded;
                return;
            }

            const RawRoom &rawBase = base.getRawCopy(id);
            const RawRoom &rawModified = mod.getRawCopy(id);
            if (rawBase != rawModified) {
                ++tl.numRoomsChanged;
            }
        },
        merge_tls);

    return result;
}

void displayRoom(AnsiOstream &os, const RoomHandle &r, const RoomFieldFlags fieldset)
{
    if (!r.exists()) {
        os << "Error: Room does not exist.\n";
        return;
    }

    // const Map &map = r.getMap();

    // TODO: convert to RawAnsi at config load time.
    static auto toRawAnsi = [](const QString &str) -> RawAnsi {
        return mmqt::parseAnsiColor(RawAnsi{}, C_ESC + str).value_or(RawAnsi{});
    };

    const auto &config = getConfig();
    if (fieldset.contains(RoomFieldEnum::NAME)) {
        const RawAnsi color = toRawAnsi(config.parser.roomNameColor);
        os.writeWithColor(color, r.getName());
        os << C_NEWLINE;
    }
    if (fieldset.contains(RoomFieldEnum::DESC)) {
        const RawAnsi color = toRawAnsi(config.parser.roomDescColor);
        os.writeWithColor(color, r.getDescription());
    }
    if (fieldset.contains(RoomFieldEnum::CONTENTS)) {
        RawAnsi color;
        color.setItalic();
        os.writeWithColor(color, r.getContents());
    }
    if (fieldset.contains(RoomFieldEnum::NOTE)) {
        if (const auto &note = r.getNote(); !note.empty()) {
            {
                RawAnsi color;
                color.setBold();
                os.writeWithColor(color, "Note");
            }

            os << ":";

            const auto &noteStr = note.getStdStringViewUtf8();
            if (countLines(noteStr) == 1) {
                /* special case for one line */
                os << " ";
                RawAnsi color;
                color.setItalic();
                os.writeWithColor(color, noteStr);
            } else {
                os << "\n";
                RawAnsi color;
                color.setItalic();
                foreachLine(noteStr, [&os, color](std::string_view line) {
                    if (line.empty()) {
                        return;
                    }

                    trim_newline_inplace(line);

                    os << "  ";
                    os.writeWithColor(color, line);
                    os << C_NEWLINE;
                });
            }
        }
    }
}

NODISCARD static std::vector<std::string> getExitKeywords(const Map &map,
                                                          const RoomId sourceId,
                                                          const ExitDirEnum i,
                                                          const RawExit &e)
{
    if (!getConfig().mumeNative.showHiddenExitFlags) {
        return {};
    }

    std::vector<std::string> etmp;
    auto add_exit_keyword = [&etmp](std::string_view word) { etmp.emplace_back(word); };

    const ExitFlags ef = e.getExitFlags();

    // Extract hidden exit flags
    /* TODO: const char* lowercaseName(ExitFlagEnum) */
    if (ef.contains(ExitFlagEnum::NO_FLEE)) {
        add_exit_keyword("noflee");
    }
    if (ef.contains(ExitFlagEnum::RANDOM)) {
        add_exit_keyword("random");
    }
    if (ef.contains(ExitFlagEnum::SPECIAL)) {
        add_exit_keyword("special");
    }
    if (ef.contains(ExitFlagEnum::DAMAGE)) {
        add_exit_keyword("damage");
    }
    if (ef.contains(ExitFlagEnum::FALL)) {
        add_exit_keyword("fall");
    }
    if (ef.contains(ExitFlagEnum::GUARDED)) {
        add_exit_keyword("guarded");
    }

    // Exit modifiers
    if (e.containsOut(sourceId)) {
        add_exit_keyword("loop");

    } else if (!e.outIsEmpty()) {
        // Check target room for exit information
        const auto targetId = e.outFirst();
        uint32_t exitCount = 0;
        bool oneWay = false;
        bool hasNoFlee = false;

        if (const auto &targetRoom = map.findRoomHandle(targetId)) {
            if (!targetRoom.getExit(opposite(i)).containsOut(sourceId)) {
                oneWay = true;
            }
            for (const ExitDirEnum j : ALL_EXITS_NESWUD) {
                const auto &targetExit = targetRoom.getExit(j);
                if (!targetExit.exitIsExit()) {
                    continue;
                }
                ++exitCount;
                if (targetExit.containsOut(sourceId)) {
                    // Technically rooms can point back in a different direction
                    oneWay = false;
                }
                if (targetExit.exitIsNoFlee()) {
                    hasNoFlee = true;
                }
            }
            if (oneWay) {
                add_exit_keyword("oneway");
            }
            if (hasNoFlee && exitCount == 1) {
                // If there is only 1 exit out of this room add the 'hasnoflee' flag since its usually a mobtrap
                add_exit_keyword("hasnoflee");
            }

            const auto loadFlags = targetRoom.getLoadFlags();
            if (loadFlags.contains(RoomLoadFlagEnum::ATTENTION)) {
                add_exit_keyword("attention");
            } else if (loadFlags.contains(RoomLoadFlagEnum::DEATHTRAP)) {
                // Override all other flags
                return {"deathtrap"};
            }

            const auto mobFlags = targetRoom.getMobFlags();
            if (mobFlags.contains(RoomMobFlagEnum::SUPER_MOB)) {
                add_exit_keyword("smob");
            }
            if (mobFlags.contains(RoomMobFlagEnum::RATTLESNAKE)) {
                add_exit_keyword("rattlesnake");
            }

            // Terrain type exit modifiers
            const RoomTerrainEnum targetTerrain = targetRoom.getTerrainType();
            if (targetTerrain == RoomTerrainEnum::UNDERWATER) {
                add_exit_keyword("underwater");
            }
        }
    }
    return etmp;
}

void enhanceExits(AnsiOstream &os, const RoomHandle &sourceRoom)
{
    if (!sourceRoom.exists()) {
        return;
    }

    const Map &map = sourceRoom.getMap();

    bool enhancedExits = false;
    std::string_view prefix = " - ";

    const auto sourceId = sourceRoom.getId();
    for (const ExitDirEnum i : ALL_EXITS_NESWUD) {
        const auto &e = sourceRoom.getExit(i);
        const ExitFlags ef = e.getExitFlags();
        if (!ef.isExit()) {
            continue;
        }

        const auto keywords = getExitKeywords(map, sourceId, i, e);
        const auto &dn = e.getDoorName();

        if ((e.doorIsHidden() && !dn.empty()) || !keywords.empty()) {
            enhancedExits = true;
            os << std::exchange(prefix, string_consts::SV_SPACE);
            os << Mmapper2Exit::charForDir(i) << C_COLON;
            if (!dn.empty()) {
                os.writeWithColor(yellow, dn.getStdStringViewUtf8());
            }
            if (!keywords.empty()) {
                auto optcomma = "";
                os << C_OPEN_PARENS;
                for (const auto &kw : keywords) {
                    os << std::exchange(optcomma, string_consts::S_COMMA);
                    os.writeWithColor(yellow, kw);
                }
                os << C_CLOSE_PARENS;
            }
        }
    }

    if (enhancedExits) {
        os << C_PERIOD;
    }
    os << C_NEWLINE;
}

void displayExits(AnsiOstream &os, const RoomHandle &r, const char sunCharacter)
{
    const Map &map = r.getMap();
    const bool hasExits = [&r]() {
        for (const ExitDirEnum direction : ALL_EXITS_NESWUD) {
            const auto &e = r.getExit(direction);
            if (e.exitIsExit()) {
                return true;
            }
        }
        return false;
    }();

    auto prefix = " ";

    os << "Exits" << C_OPEN_PARENS;
    os.writeWithColor(yellow, "emulated");
    os << C_CLOSE_PARENS << C_COLON;

    if (!hasExits) {
        os << " none.\n";
        return;
    }

    for (const ExitDirEnum direction : ALL_EXITS_NESWUD) {
        bool door = false;
        bool road = false;
        bool trail = false;
        bool climb = false;
        bool directSun = false;
        bool swim = false;

        const auto &e = r.getExit(direction);
        if (e.exitIsExit()) {
            os << std::exchange(prefix, ", ");

            const RoomTerrainEnum sourceTerrain = r.getTerrainType();
            if (!e.outIsEmpty()) {
                const auto targetId = e.outFirst();
                if (const auto &targetRoom = map.findRoomHandle(targetId)) {
                    const RoomTerrainEnum targetTerrain = targetRoom.getTerrainType();

                    // Sundeath exit flag modifiers
                    if (targetRoom.getSundeathType() == RoomSundeathEnum::SUNDEATH) {
                        directSun = true;
                        os << sunCharacter;
                    }

                    // Terrain type exit modifiers
                    if (targetTerrain == RoomTerrainEnum::RAPIDS
                        || targetTerrain == RoomTerrainEnum::UNDERWATER
                        || targetTerrain == RoomTerrainEnum::WATER) {
                        swim = true;
                        os << "~";

                    } else if (targetTerrain == RoomTerrainEnum::ROAD
                               && sourceTerrain == RoomTerrainEnum::ROAD) {
                        road = true;
                        os << "=";
                    }
                }
            }

            if (!road && e.exitIsRoad()) {
                if (sourceTerrain == RoomTerrainEnum::ROAD) {
                    road = true;
                    os << "=";
                } else {
                    trail = true;
                    os << "-";
                }
            }

            if (e.exitIsDoor()) {
                door = true;
                os << "{";
            } else if (e.exitIsClimb()) {
                climb = true;
                os << "|";
            }

            os << lowercaseDirection(direction);
        }

        if (door) {
            os << "}";
        } else if (climb) {
            os << "|";
        }
        if (swim) {
            os << "~";
        } else if (road) {
            os << "=";
        } else if (trail) {
            os << "-";
        }
        if (directSun) {
            os << sunCharacter;
        }
    }

    os << C_PERIOD;
    enhanceExits(os, r);
}

void previewRoom(AnsiOstream &os, const RoomHandle &r)
{
    displayRoom(os, r, RoomFieldEnum::NAME | RoomFieldEnum::DESC | RoomFieldEnum::CONTENTS);
    displayExits(os, r, C_ASTERISK);
    displayRoom(os, r, RoomFieldFlags{RoomFieldEnum::NOTE});
}

NODISCARD std::string previewRoom(const RoomHandle &r)
{
    std::ostringstream os;
    {
        AnsiOstream aos{os};
        previewRoom(aos, r);
    }
    return std::move(os).str();
}

namespace mmqt {
QString previewRoom(const RoomHandle &room,
                    const StripAnsiEnum stripAnsi,
                    const PreviewStyleEnum previewStyle)
{
    const auto pos = room.getPosition();
    auto desc = ::mmqt::toQStringUtf8(previewRoom(room));
    if (stripAnsi == StripAnsiEnum::Yes) {
        ::ParserUtils::removeAnsiMarksInPlace(desc);
    }

    QString room_string = QString("%1").arg(room.getIdExternal().asUint32());
    if (auto server_id = room.getServerId(); server_id != INVALID_SERVER_ROOMID) {
        room_string += QString(" (server ID %1)").arg(server_id.asUint32());
    }
    if (const RoomArea &area = room.getArea(); !area.empty()) {
        room_string += QString(" in %1").arg(area.toQString());
    }

    return QString("%1 Room %2 at Coordinates (%3, %4, %5)\n%6%7")
        .arg(previewStyle == PreviewStyleEnum::ForDisplay ? "###" : "Selected", room_string)
        .arg(pos.x)
        .arg(pos.y)
        .arg(pos.z)
        .arg(previewStyle == PreviewStyleEnum::ForDisplay ? "\n" : "", desc);
}
} // namespace mmqt

namespace { // anonymous
void testRawFlags()
{
    RawRoom rr;
    {
        auto &east = rr.getExit(ExitDirEnum::EAST);

        TEST_ASSERT(!east.getExitFlags().isClimb());

        east.addExitFlags(ExitFlagEnum::CLIMB);
        TEST_ASSERT(east.getExitFlags().isClimb());

        east.removeExitFlags(ExitFlagEnum::CLIMB);
        TEST_ASSERT(!east.getExitFlags().isClimb());

        east.addExitFlags(ExitFlags{ExitFlagEnum::CLIMB});
        TEST_ASSERT(east.getExitFlags().isClimb());

        east.removeExitFlags(ExitFlags{ExitFlagEnum::CLIMB});
        TEST_ASSERT(!east.getExitFlags().isClimb());

        rr.addExitFlags(ExitDirEnum::EAST, ExitFlagEnum::CLIMB);
        TEST_ASSERT(east.getExitFlags().isClimb());

        rr.removeExitFlags(ExitDirEnum::EAST, ExitFlagEnum::CLIMB);
        TEST_ASSERT(!east.getExitFlags().isClimb());

        rr.addExitFlags(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::CLIMB});
        TEST_ASSERT(east.getExitFlags().isClimb());

        rr.removeExitFlags(ExitDirEnum::EAST, ExitFlags{ExitFlagEnum::CLIMB});
        TEST_ASSERT(!east.getExitFlags().isClimb());
    }
    {
        TEST_ASSERT(!rr.getLoadFlags().contains(RoomLoadFlagEnum::ARMOUR));

        rr.addLoadFlags(RoomLoadFlagEnum::ARMOUR);
        TEST_ASSERT(rr.getLoadFlags().contains(RoomLoadFlagEnum::ARMOUR));

        rr.removeLoadFlags(RoomLoadFlagEnum::ARMOUR);
        TEST_ASSERT(!rr.getLoadFlags().contains(RoomLoadFlagEnum::ARMOUR));

        rr.addLoadFlags(RoomLoadFlags{RoomLoadFlagEnum::ARMOUR});
        TEST_ASSERT(rr.getLoadFlags().contains(RoomLoadFlagEnum::ARMOUR));

        rr.removeLoadFlags(RoomLoadFlags{RoomLoadFlagEnum::ARMOUR});
        TEST_ASSERT(!rr.getLoadFlags().contains(RoomLoadFlagEnum::ARMOUR));
    }
    {
        TEST_ASSERT(!rr.getMobFlags().contains(RoomMobFlagEnum::RENT));

        rr.addMobFlags(RoomMobFlagEnum::RENT);
        TEST_ASSERT(rr.getMobFlags().contains(RoomMobFlagEnum::RENT));

        rr.removeMobFlags(RoomMobFlagEnum::RENT);
        TEST_ASSERT(!rr.getMobFlags().contains(RoomMobFlagEnum::RENT));

        rr.addMobFlags(RoomMobFlags{RoomMobFlagEnum::RENT});
        TEST_ASSERT(rr.getMobFlags().contains(RoomMobFlagEnum::RENT));

        rr.removeMobFlags(RoomMobFlags{RoomMobFlagEnum::RENT});
        TEST_ASSERT(!rr.getMobFlags().contains(RoomMobFlagEnum::RENT));
    }
}

void testAddAndRemoveIsNoChange()
{
    ProgressCounter pc;

    const Coordinate firstCoord{0, 0, 0};
    const Coordinate secondCoord{1, 1, 0};
    TEST_ASSERT(secondCoord != firstCoord);

    auto rooms = std::vector<ExternalRawRoom>{};
    auto &room = rooms.emplace_back();
    room.id = ExternalRoomId{0};
    room.status = RoomStatusEnum::Permanent;
    room.setName(RoomName{"Name"});
    room.setPosition(firstCoord);

    std::vector<InfoMarkFields> marks;
    auto &im = marks.emplace_back();
    im.setType(InfoMarkTypeEnum::TEXT);
    im.setText(InfoMarkText{"Text"});

    const MapPair mapPair = Map::fromRooms(pc, rooms, marks);
    TEST_ASSERT(mapPair.base == mapPair.modified);
    const Map &m1 = mapPair.modified;
    TEST_ASSERT(m1.getRoomsCount() == 1);
    TEST_ASSERT(m1.getMarksCount() == 1);

    const auto firstChangeResult = m1.applySingleChange(pc,
                                                        Change{room_change_types::AddPermanentRoom{
                                                            secondCoord}});
    TEST_ASSERT(firstChangeResult.roomUpdateFlags & RoomUpdateEnum::BoundsChanged);
    TEST_ASSERT(firstChangeResult.roomUpdateFlags & RoomUpdateEnum::RoomMeshNeedsUpdate);

    const Map &m2 = firstChangeResult.map;
    TEST_ASSERT(m2 != m1);
    TEST_ASSERT(m2.getRoomsCount() == 2);

    const auto secondId = [&m2, &secondCoord]() -> RoomId {
        // REVISIT: add should we add Map::findRoomId(Coordinate)?
        if (const auto r2 = m2.findRoomHandle(secondCoord)) {
            return r2.getId();
        }
        return INVALID_ROOMID;
    }();
    TEST_ASSERT(secondId != INVALID_ROOMID);
    const auto secondChangeResult = m2.applySingleChange(pc,
                                                         Change{room_change_types::RemoveRoom{
                                                             secondId}});
    TEST_ASSERT(secondChangeResult.roomUpdateFlags & RoomUpdateEnum::BoundsChanged);
    TEST_ASSERT(secondChangeResult.roomUpdateFlags & RoomUpdateEnum::RoomMeshNeedsUpdate);

    const Map &m3 = secondChangeResult.map;
    TEST_ASSERT(m3.getRoomsCount() == 1);
    TEST_ASSERT(m3 != m2);
    TEST_ASSERT(m3 == m1);

    {
        const auto stats12 = World::getComparisonStats(m1.getWorld(), m2.getWorld());
        TEST_ASSERT(stats12.boundsChanged);
        TEST_ASSERT(!stats12.anyRoomsRemoved);
        TEST_ASSERT(stats12.anyRoomsAdded);
        TEST_ASSERT(stats12.spatialDbChanged);
        TEST_ASSERT(stats12.hasMeshDifferences);
        TEST_ASSERT(!stats12.serverIdsChanged);
    }

    {
        const auto stats23 = World::getComparisonStats(m2.getWorld(), m3.getWorld());
        TEST_ASSERT(stats23.boundsChanged);
        TEST_ASSERT(stats23.anyRoomsRemoved);
        TEST_ASSERT(!stats23.anyRoomsAdded);
        TEST_ASSERT(stats23.spatialDbChanged);
        TEST_ASSERT(stats23.hasMeshDifferences);
        TEST_ASSERT(!stats23.serverIdsChanged);
    }

    {
        const auto stats13 = World::getComparisonStats(m1.getWorld(), m3.getWorld());
        TEST_ASSERT(!stats13.boundsChanged);
        TEST_ASSERT(!stats13.anyRoomsRemoved);
        TEST_ASSERT(!stats13.anyRoomsAdded);
        TEST_ASSERT(!stats13.spatialDbChanged);
        TEST_ASSERT(!stats13.hasMeshDifferences);
        TEST_ASSERT(!stats13.serverIdsChanged);
    }
}

constexpr auto defaultAlign = enums::getInvalidValue<RoomAlignEnum>();
constexpr auto goodAlign = RoomAlignEnum::GOOD;
constexpr auto errorAlign = static_cast<RoomAlignEnum>(255);

static_assert(defaultAlign != goodAlign);
static_assert(defaultAlign != errorAlign);
static_assert(goodAlign != errorAlign);
static_assert(enums::isValidEnumValue(defaultAlign));
static_assert(enums::isValidEnumValue(goodAlign));
static_assert(!enums::isValidEnumValue(errorAlign));
static_assert(enums::sanitizeEnum(errorAlign) == defaultAlign);

void testAddingInvalidEnums()
{
    ProgressCounter pc;
    Map map;
    const auto result1 = map.applySingleChange(pc,
                                               Change{room_change_types::AddPermanentRoom{
                                                   Coordinate{}}});
    TEST_ASSERT(result1.map.getRoomsCount() == 1);
    map = result1.map;
    const RoomId room = map.getRooms().first();
    TEST_ASSERT(room != INVALID_ROOMID);

    auto getAlign = [room, &map]() -> RoomAlignEnum { return map.getRawRoom(room).getAlignType(); };

    auto setAlign = [room, &map, &pc](const RoomAlignEnum newAlign) {
        const auto result2 = map.applySingleChange(
            pc,
            Change{room_change_types::ModifyRoomFlags{room, newAlign, FlagModifyModeEnum::ASSIGN}});
        map = result2.map;
    };

    TEST_ASSERT(getAlign() == defaultAlign);
    setAlign(goodAlign);
    TEST_ASSERT(getAlign() == goodAlign);
    setAlign(errorAlign);
    TEST_ASSERT(getAlign() == defaultAlign);
}

void testConstructingInvalidEnums()
{
    auto testcase = [](const RoomAlignEnum set, const RoomAlignEnum expect) {
        ProgressCounter pc;
        ExternalRawRoom raw;
        raw.id = ExternalRoomId{1};
        raw.setAlignType(set);

        auto map = Map::fromRooms(pc, {raw}, {});
        TEST_ASSERT(map.base.getRoomsCount() == 1);
        TEST_ASSERT(map.base.getMarksCount() == 0);

        const auto roomId = map.base.getRooms().first();
        const auto raw2 = map.base.getRawRoom(roomId);
        TEST_ASSERT(raw2.getAlignType() == expect);
    };

    testcase(defaultAlign, defaultAlign);
    testcase(goodAlign, goodAlign);
    testcase(errorAlign, defaultAlign);
}

void testDoorVsExitFlags()
{
    ProgressCounter pc;
    Map map;
    for (int x = 0; x < 2; ++x) {
        const auto result1 = map.applySingleChange(pc,
                                                   Change{room_change_types::AddPermanentRoom{
                                                       Coordinate{x, 0, 0}}});
        map = result1.map;
    }
    TEST_ASSERT(map.getRoomsCount() == 2);
    const RoomId from = map.getRooms().first();
    const RoomId to = *(std::next(map.getRooms().begin()));
    TEST_ASSERT(from != to);

    auto getExit = [from, &map]() -> const RawExit & {
        const RawRoom &raw = map.getRawRoom(from);
        return raw.getExit(ExitDirEnum::EAST);
    };

    auto createExit = [from, to, &map, &pc]() {
        const auto result
            = map.applySingleChange(pc,
                                    Change{
                                        exit_change_types::ModifyExitConnection{ChangeTypeEnum::Add,
                                                                                from,
                                                                                ExitDirEnum::EAST,
                                                                                to,
                                                                                WaysEnum::TwoWay}});
        map = result.map;
    };

    auto setDoor = [from, &map, &pc](bool set) {
        const auto result = map.applySingleChange(
            pc,
            Change{
                exit_change_types::SetExitFlags{set ? FlagChangeEnum::Set : FlagChangeEnum::Remove,
                                                from,
                                                ExitDirEnum::EAST,
                                                ExitFlags{ExitFlagEnum::DOOR}}});
        map = result.map;
    };

    auto setHidden = [from, &map, &pc](bool set) {
        const auto result = map.applySingleChange(
            pc,
            Change{
                exit_change_types::SetDoorFlags{set ? FlagChangeEnum::Set : FlagChangeEnum::Remove,
                                                from,
                                                ExitDirEnum::EAST,
                                                DoorFlags{DoorFlagEnum::HIDDEN}}});
        map = result.map;
    };

    enum class NODISCARD ExpectDoorEnum : uint8_t { None, Visible, Hidden };

    auto check = [&getExit](const std::optional<ExpectDoorEnum> expect) {
        const bool isExit = expect.has_value();
        const bool isDoor = isExit && expect != ExpectDoorEnum::None;
        const bool isHidden = isExit && expect == ExpectDoorEnum::Hidden;
        const RawExit &east = getExit();
        TEST_ASSERT(east.fields.exitFlags.isExit() == isExit);
        TEST_ASSERT(east.fields.exitFlags.isDoor() == isDoor);
        TEST_ASSERT(east.fields.doorFlags.isHidden() == isHidden);
    };

    // trying to hide an exit that doesn't exist = no change
    check(std::nullopt);
    setHidden(true);
    check(std::nullopt);
    setDoor(true);
    check(std::nullopt);

    // the exit exists once it's created
    createExit();
    check(ExpectDoorEnum::None);

    // setting hidden makes it *both* door and hidden
    setHidden(true);
    check(ExpectDoorEnum::Hidden);

    // removing hidden only removes the hidden attribute
    setHidden(false);
    check(ExpectDoorEnum::Visible);
    setHidden(true);
    check(ExpectDoorEnum::Hidden);

    // removing door while already hidden = no change!
    setDoor(false);
    check(ExpectDoorEnum::Hidden);
    setDoor(true);
    check(ExpectDoorEnum::Hidden);

    // removing hidden first and then removing door removes both attributes
    setHidden(false);
    setDoor(false);
    check(ExpectDoorEnum::None);
}

} // namespace

namespace test {
void testMap()
{
    Map::enableExtraSanityChecks(true);
    testRoomIdSet();
    testRawFlags();
    testAddAndRemoveIsNoChange();
    testMapEnums();
    testAddingInvalidEnums();
    testConstructingInvalidEnums();
    testDoorVsExitFlags();
    test::test_mmapper2room();
}
} // namespace test
