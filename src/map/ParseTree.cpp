// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "ParseTree.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/LineUtils.h"
#include "../global/PrintUtils.h"
#include "../global/logging.h"
#include "Compare.h"
#include "Map.h"
#include "RoomRecipient.h"
#include "World.h"
#include "sanitizer.h"

#include <deque>
#include <optional>
#include <ostream>
#include <sstream>

void getRooms(const Map &map, const ParseTree &tree, RoomRecipient &visitor, const ParseEvent &event)
{
    using Clock = std::chrono::steady_clock;
    const auto t0 = Clock::now();
    static volatile bool fallbackToWholeMap = true;

    const RoomIdSet *const pSet = [&map, &event, &tree]() -> const RoomIdSet * {
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

        if (fallbackToWholeMap) {
            MMLOG() << "[getRooms] Failed to find a match with v1. Falling back to the whole map!";
            MMLOG() << "[getRooms] event: " << mmqt::toStdStringUtf8(event.toQString());
            return std::addressof(map.getWorld().getRoomSet());
        } else {
            MMLOG() << "[getRooms] Failed to find a match with v1; giving up.";
            MMLOG() << "[getRooms] event: " << mmqt::toStdStringUtf8(event.toQString());
            return nullptr;
        }
    }();

    const auto t1 = Clock::now();

    if (pSet == nullptr) {
        MMLOG() << "[getRooms] Unable to find any matches.";
        return;
    }

    auto &set = *pSet;

    const int tolerance = getConfig().pathMachine.matchingTolerance;
    auto tryReport = [&event, &visitor, tolerance](const RoomHandle &room) {
        if (fallbackToWholeMap) {
            if (::compare(room.getRaw(), event, tolerance) == ComparisonResultEnum::DIFFERENT) {
                return false;
            }
        }

        // FIXME "Inside Grey Havens" (arda.mm2) vs "Inside the Grey Havens" (MUME)
        // e.g. Don't let DEATHTRAP rooms be converted to "Inside the Grey Havens",
        // and definitely don't choose name="", desc="" over "Inside Grey Havens"!
        visitor.receiveRoom(room);
        return true;
    };

    MMLOG() << "[getRooms] Found " << set.size() << " potential match(es).";

    const auto t2 = Clock::now();
    size_t numReported = 0;
    for (const RoomId id : set) {
        if (const auto optRoom = map.findRoomHandle(id)) {
            if (tryReport(optRoom)) {
                ++numReported;
            }
        }
    }
    const auto t3 = Clock::now();

    MMLOG() << "[getRooms] Reported " << numReported << " potential match(es).";

    auto &&os = MMLOG();
    os << "[getRooms] timing follows:\n";

    auto report = [&os](std::string_view what, Clock::time_point a, Clock::time_point b) {
        auto ms = static_cast<double>(static_cast<std::chrono::nanoseconds>(b - a).count()) * 1e-6;
        os << "[TIMER] [getRooms] " << what << ": " << ms << " ms\n";
    };

    report("part0. lookup rooms", t0, t1);
    report("part1. (nothing)", t1, t2);
    report("part2. for(...) tryReport() ", t2, t3);
    report("overall", t0, t3);
}

void ParseTree::printStats(ProgressCounter & /*pc*/, AnsiOstream &os) const
{
    static constexpr RawAnsi green = getRawAnsi(AnsiColor16Enum::green);
    static constexpr RawAnsi yellow = getRawAnsi(AnsiColor16Enum::yellow);

    {
        const size_t total_name = name_only.size();
        const size_t total_desc = desc_only.size();
        const size_t total_name_desc = name_desc.size();

        os << "\n";
        os << "Total name combinations:              " << total_name << ".\n";
        os << "Total desc combinations:              " << total_desc << ".\n";
        os << "Total name+desc combinations:         " << total_name_desc << ".\n";

        const auto countUnique = [](const auto &map) -> size_t {
            return static_cast<size_t>(std::count_if(std::begin(map), std::end(map), [](auto &kv) {
                return kv.second.size() == 1;
            }));
        };

        const size_t unique_name = countUnique(name_only);
        const size_t unique_desc = countUnique(desc_only);
        const size_t unique_name_Desc = countUnique(name_desc);

        os << "\n";
        os << "  unique name:              " << unique_name << ".\n";
        os << "  unique desc:              " << unique_desc << ".\n";
        os << "  unique name+desc:         " << unique_name_Desc << ".\n";

        os << "\n";
        os << "  non-unique names:             " << (total_name - unique_name) << ".\n";
        os << "  non-unique descs:             " << (total_desc - unique_desc) << ".\n";
        os << "  non-unique name+desc:         " << (total_name_desc - unique_name_Desc) << ".\n";
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
        os << "  rooms w/ non-unique names:             " << nonunique_name << ".\n";
        os << "  rooms w/ non-unique descs:             " << nonunique_desc << ".\n";
        os << "  rooms w/ non-unique name+desc:         " << nonunique_name_desc << ".\n";
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

        if (mostCommon) {
            os << "\n";
            os << "Most common non-default " << thing << " appears " << mostCommonCount << " time"
               << ((mostCommonCount == 1) ? "" : "s") << ":\n";

            DisplayHelper::print(os, *mostCommon);
        }
    };
    printMostCommon(name_only, "name");
    printMostCommon(desc_only, "desc");
    printMostCommon(name_desc, "name+desc");
}
