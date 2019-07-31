// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "basemapsavefilter.h"

#include <cassert>
#include <list>
#include <set>
#include <QtCore>

#include "../expandoracommon/exit.h"
#include "../expandoracommon/room.h"
#include "../global/roomid.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomfilter.h"
#include "progresscounter.h"

class RoomAdmin;

namespace {

bool contains(const std::set<RoomId> &set, RoomId x)
{
    return set.find(x) == set.end();
}

struct RoomLink
{
    RoomId from = INVALID_ROOMID, to = INVALID_ROOMID;
    explicit RoomLink(RoomId f, RoomId t)
        : from(f)
        , to(t)
    {}

public:
    RoomLink() = delete;
};

bool operator<(const RoomLink &a, const RoomLink &b)
{
    if (a.from != b.from) {
        return a.from < b.from;
    }
    return a.to < b.to;
}
} // namespace

struct BaseMapSaveFilter::Impl
{
    //! Owned by caller
    MapData *mapData = nullptr;

    //! Rooms reachable without going through hidden doors
    RoomIdSet baseRooms;

    /*! \brief Rooms scheduled for exploration during prepare
     *
     * Order matters (can't be replaced by a set) to prevent infinite looping.
     */
    std::list<RoomId> roomsTodo;

    /*! \brief Secret links noticed during the exploration (prepare).
     *
     * This data is used to remove secret links between public rooms (example:
     * hedge on OER near Bree). As such it doesn't include secret links between
     * secret rooms, but this doesn't matter.
     */
    std::set<RoomLink> secretLinks{};
};

BaseMapSaveFilter::BaseMapSaveFilter()
    : m_impl(new Impl)
{}

BaseMapSaveFilter::~BaseMapSaveFilter() = default;

void BaseMapSaveFilter::prepare(ProgressCounter &counter)
{
    assert(m_impl->mapData);

    std::set<RoomId> considered{};

    // Seed room
    m_impl->mapData->genericSearch(this,
                                   RoomFilter("The Fountain Square",
                                              Qt::CaseSensitive,
                                              PatternKindsEnum::NAME));

    // Walk the whole map through non-hidden exits without recursing
    while (!m_impl->roomsTodo.empty()) {
        auto todo = m_impl->roomsTodo.front();
        m_impl->roomsTodo.pop_front();

        if (considered.find(todo)
            == considered.end()) { // Don't process twice the same room (ending condition)
            m_impl->mapData->lookingForRooms(*this, todo);
            m_impl->mapData->releaseRoom(*this, todo);
            considered.insert(todo);
            counter.step();
        }
    }

    for (auto steps = considered.size(); steps < prepareCount(); ++steps) {
        counter.step(); // Make up for the secret rooms we skipped
    }
}

void BaseMapSaveFilter::receiveRoom(RoomAdmin * /*admin*/, const Room *room)
{
    for (const auto &exit : room->getExitsList()) {
        for (auto to : exit.outRange()) {
            if (exit.isHiddenExit()) {
                m_impl->secretLinks.emplace(RoomLink(room->getId(), to));
            } else {
                m_impl->baseRooms.emplace(to);
                m_impl->roomsTodo.emplace_back(to);
            }
        }
    }
}

void BaseMapSaveFilter::setMapData(MapData *mapData)
{
    m_impl->mapData = mapData;
}

uint32_t BaseMapSaveFilter::prepareCount()
{
    assert(m_impl->mapData);
    return m_impl->mapData->getRoomsCount();
}

uint32_t BaseMapSaveFilter::acceptedRoomsCount()
{
    return static_cast<uint32_t>(m_impl->baseRooms.size());
}

BaseMapSaveFilter::ActionEnum BaseMapSaveFilter::filter(const Room &room)
{
    auto &baseRooms = m_impl->baseRooms;
    assert(!baseRooms.empty());

    if (baseRooms.find(room.getId()) != baseRooms.end()) {
        const ExitsList &exits = room.getExitsList();
        for (const auto &exit : exits) {
            if (exit.isHiddenExit()) {
                return ActionEnum::ALTER;
            }
            for (auto idx : exit.outRange()) {
                if (contains(baseRooms, idx)) {
                    return ActionEnum::ALTER;
                }
            }
            for (auto idx : exit.inRange()) {
                if (contains(baseRooms, idx)) {
                    return ActionEnum::ALTER;
                }
            }
        }

        return ActionEnum::PASS;
    }
    return ActionEnum::REJECT;
}

Room BaseMapSaveFilter::alteredRoom(const Room &room)
{
    auto &baseRooms = m_impl->baseRooms;
    auto &secretLinks = m_impl->secretLinks;
    assert(!baseRooms.empty());

    Room copy = room;

    ExitsList &exits = copy.getExitsList();
    for (auto &exit : exits) {
        auto outLinks = exit.outClone();
        auto inLinks = exit.inClone();

        // Destroy links to secret rooms
        for (auto outLink : outLinks) {
            const bool destRoomIsSecret = contains(baseRooms, outLink);
            const bool outLinkIsSecret = (secretLinks.find(RoomLink(copy.getId(), outLink))
                                          != secretLinks.end());
            const bool linkBackIsSecret = (secretLinks.find(RoomLink(outLink, copy.getId()))
                                           != secretLinks.end());

            if (destRoomIsSecret || (outLinkIsSecret && linkBackIsSecret)) {
                exit.removeOut(outLink);

                exit.clearFields();
            }
        }

        // Destroy links from secret rooms to here
        for (auto inLink : inLinks) {
            if (contains(baseRooms, inLink)) {
                exit.removeIn(inLink);
            }
        }

        // Remove names on hidden exits to areas reachable through visible doors
        if (exit.isHiddenExit()) {
            exit.clearDoorName();
        }
    }

    return copy;
}
