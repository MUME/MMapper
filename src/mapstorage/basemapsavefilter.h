#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "../expandoracommon/RoomRecipient.h"

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
class BaseMapSaveFilter : public RoomRecipient
{
public:
    enum class Action { PASS, ALTER, REJECT };

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // Disallow copying because unique_ptr is used
    BaseMapSaveFilter(const BaseMapSaveFilter &);
    BaseMapSaveFilter &operator=(const BaseMapSaveFilter &);

    virtual void receiveRoom(RoomAdmin *, const Room *room) override;

public:
    explicit BaseMapSaveFilter();
    virtual ~BaseMapSaveFilter() override;

    //! The map data to work on
    void setMapData(MapData *mapData);
    //! How much steps (rooms) to go through in prepare() (requires mapData)
    uint32_t prepareCount();
    //! How much rooms will be accepted (PASS or ALTER)
    uint32_t acceptedRoomsCount();
    //! First pass over the world's room (requires mapData)
    void prepare(ProgressCounter &counter);
    //! Determines the fate of this room (requires prepare())
    Action filter(const Room &room);
    //! Returns an altered Room (requires action == ALTER)
    Room alteredRoom(const Room &room);
};
