#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/room.h"
#include "../global/RuleOf5.h"

#include <cstdint>
#include <memory>

#include <sys/types.h>

class RoomAdmin;
class MapData;
class Room;
class ProgressCounter;

/*! \brief Filters
 *
 */
class NODISCARD BaseMapSaveFilter final : public RoomRecipient
{
public:
    enum class NODISCARD ActionEnum { PASS, ALTER, REJECT };

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

private:
    virtual void virt_receiveRoom(RoomAdmin *, const Room *room) final;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(BaseMapSaveFilter);

public:
    BaseMapSaveFilter();
    ~BaseMapSaveFilter() final;

public:
    //! The map data to work on
    void setMapData(MapData *mapData);
    //! How much steps (rooms) to go through in prepare() (requires mapData)
    NODISCARD uint32_t prepareCount();
    //! How much rooms will be accepted (PASS or ALTER)
    NODISCARD uint32_t acceptedRoomsCount();
    //! First pass over the world's room (requires mapData)
    void prepare(ProgressCounter &counter);
    //! Determines the fate of this room (requires prepare())
    ActionEnum filter(const Room &room);

private:
    //! Returns an altered Room (requires action == ALTER)
    std::shared_ptr<Room> alteredRoom(RoomModificationTracker &tracker, const Room &room);
    bool isSecret(RoomId) const;

public:
    template<typename Visitor>
    void visitRoom(const Room &room, bool const baseMapOnly, Visitor &&visit)
    {
        if (!baseMapOnly) {
            visit(room);
            return;
        }

        switch (this->filter(room)) {
        case ActionEnum::REJECT:
            return;
        case ActionEnum::PASS:
            visit(room);
            return;
        case ActionEnum::ALTER: {
            RoomModificationTracker rmt;
            const auto copy = this->alteredRoom(rmt, room);
            visit(deref(copy));
            copy->setAboutToDie();
            return;
        }
        }
    }
};
