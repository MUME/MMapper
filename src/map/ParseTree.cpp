// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ParseTree.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/LineUtils.h"
#include "../global/PrintUtils.h"
#include "../global/Timer.h"
#include "../global/logging.h"
#include "Compare.h"
#include "Map.h"
#include "World.h"

#include <deque>
#include <optional>
#include <string_view>

RoomIdSet getRooms(const Map &map, const ParseTree &tree, const ParseEvent &event)
{
    DECL_TIMER(t0, "overall");

    static volatile bool fallbackToCurrentArea = true;
    static volatile bool fallbackToRemainder = true;
    static volatile bool fallbackToWholeMap = true;

    const auto *const pSet = [&map, &event, &tree]() -> const ImmUnorderedRoomIdSet * {
        DECL_TIMER(t1, "part0. lookup rooms in areas");
        const World &world = map.getWorld();
        const RoomArea &areaName = event.getRoomArea();

        const RoomName &name = event.getRoomName();
        const RoomDesc &desc = event.getRoomDesc();

        const bool hasName = !name.empty();
        const bool hasDesc = !desc.empty();

        if (hasName && hasDesc) {
            if (auto set = tree.name_desc.find(NameDesc{name, desc})) {
                return set;
            }

            MMLOG() << "[getRooms] Failed to find a match with name+desc. Falling back to name or desc...";
        }

        if (hasName) {
            if (auto ptr = tree.name_only.find(name)) {
                return ptr;
            }
        }

        if (hasDesc) {
            if (auto ptr = tree.desc_only.find(desc)) {
                return ptr;
            }
        }

        if (fallbackToCurrentArea) {
            MMLOG() << "[getRooms] Falling back to the current area!";
            MMLOG() << "[getRooms] event: " << mmqt::toStdStringUtf8(event.toQString());

            if (const auto *const set = world.findAreaRoomSet(areaName); set == nullptr) {
                MMLOG() << "[getRooms] Area does not exist.";
            } else if (set->empty()) {
                MMLOG() << "[getRooms] Area was empty.";
            } else {
                return set;
            }
        }

        if (fallbackToRemainder && !areaName.empty()) {
            MMLOG() << "[getRooms] Falling back to the remainder area...";
            if (const auto *const set = world.findAreaRoomSet(RoomArea{}); set == nullptr) {
                MMLOG() << "[getRooms] Fallback area does not exist.";
            } else if (set->empty()) {
                // this should just return nullptr.
                MMLOG() << "[getRooms] Fallback area was empty.";
            } else {
                return set;
            }
        }
        return nullptr;
    }();

    const auto filterRoomsByEvent = [&map, &event](const auto &set) -> RoomIdSet {
        DECL_TIMER(t2, "part2. for(...) tryReport() ");

        const int tolerance = getConfig().pathMachine.matchingTolerance;
        auto tryReport = [&event, tolerance](const RoomHandle &room) {
            if (::compare(room.getRaw(), event, tolerance) == ComparisonResultEnum::DIFFERENT) {
                return false;
            }
            return true;
        };

        MMLOG() << "[getRooms] Found " << set.size() << " potential match(es).";

        RoomIdSet results;
        size_t numReported = 0;
        set.for_each([&](const RoomId id) {
            if (const auto optRoom = map.findRoomHandle(id)) {
                if (tryReport(optRoom)) {
                    results.insert(id);
                    ++numReported;
                }
            }
        });

        MMLOG() << "[getRooms] Reported " << numReported << " potential match(es).";
        return results;
    };

    if (pSet == nullptr) {
        DECL_TIMER(t2, "part1. fallback to whole map");
        if (fallbackToWholeMap) {
            MMLOG() << "[getRooms] Falling back to the whole map...";

            auto &set = map.getWorld().getRoomSet();
            if (set.empty()) {
                MMLOG() << "[getRooms] Failed to find a match; giving up.";
                MMLOG() << "[getRooms] event: " << mmqt::toStdStringUtf8(event.toQString());
                return {};
            }
            // this is probably unnecessary, and it's probably also the source of some bugs,
            // since it could find a room known to be in a different area.
            return filterRoomsByEvent(set);
        }

        MMLOG() << "[getRooms] Unable to find any matches.";
        return {};
    } else {
        DECL_TIMER(t2, "part1. (nothing)");
    }

    auto &set = *pSet;
    return filterRoomsByEvent(set);
}

void ParseTree::printStats(ProgressCounter & /*pc*/, AnsiOstream &os) const
{
    static constexpr RawAnsi green = getRawAnsi(AnsiColor16Enum::green);
    static constexpr RawAnsi yellow = getRawAnsi(AnsiColor16Enum::yellow);

    auto C = [](auto x) {
        static_assert(std::is_integral_v<decltype(x)>);
        return ColoredValue{green, x};
    };

    {
        const size_t total_name = name_only.size();
        const size_t total_desc = desc_only.size();
        const size_t total_name_desc = name_desc.size();

        os << "\n";
        os << "Total name combinations:              " << C(total_name) << ".\n";
        os << "Total desc combinations:              " << C(total_desc) << ".\n";
        os << "Total name+desc combinations:         " << C(total_name_desc) << ".\n";

        const auto countUnique = [](const auto &map) -> size_t {
            return static_cast<size_t>(std::count_if(std::begin(map), std::end(map), [](auto &kv) {
                return kv.second.size() == 1;
            }));
        };

        const size_t unique_name = countUnique(name_only);
        const size_t unique_desc = countUnique(desc_only);
        const size_t unique_name_Desc = countUnique(name_desc);

        os << "\n";
        os << "  unique name:              " << C(unique_name) << ".\n";
        os << "  unique desc:              " << C(unique_desc) << ".\n";
        os << "  unique name+desc:         " << C(unique_name_Desc) << ".\n";

        os << "\n";
        os << "  non-unique names:             " << C(total_name - unique_name) << ".\n";
        os << "  non-unique descs:             " << C(total_desc - unique_desc) << ".\n";
        os << "  non-unique name+desc:         " << C(total_name_desc - unique_name_Desc) << ".\n";
    }

    {
        auto count_nonunique = [](const auto &map) -> size_t {
            size_t nonunique = 0;
            for (const auto &kv : map) {
                const auto n = kv.second.size();
                if (n == 1) {
                    continue;
                }
                nonunique += n;
            }
            return nonunique;
        };

        const size_t nonunique_name = count_nonunique(name_only);
        const size_t nonunique_desc = count_nonunique(desc_only);
        const size_t nonunique_name_desc = count_nonunique(name_desc);

        os << "\n";
        os << "  rooms w/ non-unique names:             " << C(nonunique_name) << ".\n";
        os << "  rooms w/ non-unique descs:             " << C(nonunique_desc) << ".\n";
        os << "  rooms w/ non-unique name+desc:         " << C(nonunique_name_desc) << ".\n";
    }

    struct NODISCARD DisplayHelper final
    {
        static void print(AnsiOstream &os, const RoomName &name)
        {
            os.writeQuotedWithColor(green, yellow, name.getStdStringViewUtf8());
            os << "\n";
        }
        static void print(AnsiOstream &os, const RoomDesc &desc)
        {
            foreachLine(desc.getStdStringViewUtf8(), [&os](std::string_view line) {
                os.writeQuotedWithColor(green, yellow, line);
                os << "\n";
            });
        }

        static void print(AnsiOstream &os, const NameDesc &v2key)
        {
            os << "Name:\n";
            print(os, v2key.name);
            os << "Desc:\n";
            print(os, v2key.desc);
        }
    };

    auto printMostCommon = [&os](const auto &map, std::string_view thing) {
        using Map = decltype(map);
        using Pair = decltype(*std::declval<Map>().begin());
        using Key = std::remove_const_t<decltype(std::declval<Pair>().first)>;
        // using Value = decltype(std::declval<Pair>().second);

        const Key defaultKey{};
        std::optional<Key> mostCommon;
        size_t mostCommonCount = 0;

        for (const auto &kv : map) {
            if (kv.first == defaultKey) {
                continue;
            }

            const auto count = kv.second.size();
            if (!mostCommon || count > mostCommonCount) {
                mostCommon = kv.first;
                mostCommonCount = count;
            }
        }

        if (mostCommon && mostCommonCount > 1) {
            os << "\n";
            // Note: This excludes default/empty strings.
            os << "Most common " << thing << " appears " << ColoredValue{green, mostCommonCount}
               << " time" << ((mostCommonCount == 1) ? "" : "s") << ":\n";

            DisplayHelper::print(os, *mostCommon);
        }
    };
    printMostCommon(name_only, "name");
    printMostCommon(desc_only, "desc");
    printMostCommon(name_desc, "name+desc");
}
