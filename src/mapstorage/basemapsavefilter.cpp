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

struct NODISCARD RoomLink
{
    RoomId from = INVALID_ROOMID, to = INVALID_ROOMID;
    explicit RoomLink(RoomId f, RoomId t)
        : from(f)
        , to(t)
    {}

public:
    RoomLink() = delete;
    NODISCARD
    friend bool operator<(const RoomLink &a, const RoomLink &b)
    {
        if (a.from != b.from) {
            return a.from < b.from;
        }
        return a.to < b.to;
    }
};

} // namespace

struct NODISCARD BaseMapSaveFilter::Impl
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

    // It's considered secret if it's NOT FOUND in the set of rooms only reachable without going through hidden exits.
    bool isSecret(RoomId id) const { return baseRooms.find(id) == baseRooms.end(); }
};

BaseMapSaveFilter::BaseMapSaveFilter()
    : m_impl(new Impl)
{}

BaseMapSaveFilter::~BaseMapSaveFilter() = default;

bool BaseMapSaveFilter::isSecret(RoomId id) const
{
    return m_impl->isSecret(id);
}

void BaseMapSaveFilter::prepare(ProgressCounter &counter)
{
    assert(m_impl->mapData);

    std::set<RoomId> considered{};

    // Seed rooms
    static const char *seeds[] = {"The Fountain Square", "Cosy Room"};
    for (const auto &name : seeds) {
        const auto filter = RoomFilter(name, Qt::CaseSensitive, false, PatternKindsEnum::NAME);
        m_impl->mapData->genericSearch(this, filter);
    }

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

void BaseMapSaveFilter::virt_receiveRoom(RoomAdmin * /*admin*/, const Room *room)
{
    for (const auto &exit : room->getExitsList()) {
        for (auto to : exit.outRange()) {
            if (exit.isHiddenExit() || exit.exitIsNoMatch()) {
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
                if (isSecret(idx)) {
                    return ActionEnum::ALTER;
                }
            }
            for (auto idx : exit.inRange()) {
                if (isSecret(idx)) {
                    return ActionEnum::ALTER;
                }
            }
        }

        return ActionEnum::PASS;
    }
    return ActionEnum::REJECT;
}

std::shared_ptr<Room> BaseMapSaveFilter::alteredRoom(RoomModificationTracker &tracker,
                                                     const Room &room)
{
    auto &baseRooms = m_impl->baseRooms;
    auto &secretLinks = m_impl->secretLinks;
    assert(!baseRooms.empty());

    auto result = room.clone(tracker);
    Room &copy = deref(result);

    ExitsList copiedExits = copy.getExitsList();
    for (auto &exit : copiedExits) {
        auto outLinks = exit.outClone();
        auto inLinks = exit.inClone();

        // Destroy links to secret rooms
        for (auto outLink : outLinks) {
            const bool destRoomIsSecret = isSecret(outLink);
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
            if (isSecret(inLink)) {
                exit.removeIn(inLink);
            }
        }

        // Remove names on hidden exits to areas reachable through visible doors
        if (exit.isHiddenExit()) {
            exit.setDoorName(DoorName{});
        }
    }

    copy.setExitsList(copiedExits);
    return result;
}
