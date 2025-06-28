#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "Map.h"
#include "RawExit.h"
#include "RawRoom.h"

#include <vector>

class ProgressCounter;

struct NODISCARD RemovedDoorName final
{
    ExternalRoomId room;
    ExitDirEnum dir = ExitDirEnum::NONE;
    DoorName name;
};

struct NODISCARD MovedRoom final
{
    ExternalRoomId room;
    Coordinate original;
};

struct NODISCARD SanitizerChanges final
{
    std::vector<RemovedDoorName> removedDoors;
    std::vector<MovedRoom> movedRooms;
};

struct NODISCARD WorldBuilder final
{
private:
    std::vector<ExternalRawRoom> m_rooms;
    std::vector<RawInfomark> m_marks;
    ProgressCounter &m_counter;

public:
    explicit WorldBuilder(ProgressCounter &counter,
                          std::vector<ExternalRawRoom> rooms,
                          std::vector<RawInfomark> marks)
        : m_rooms{std::move(rooms)}
        , m_marks{std::move(marks)}
        , m_counter{counter}
    {}

public:
    void reserve(size_t size) { m_rooms.reserve(size); }

private:
    NODISCARD static SanitizerChanges sanitize(ProgressCounter &counter,
                                               std::vector<ExternalRawRoom> &input);
    NODISCARD static MapPair build(ProgressCounter &counter,
                                   std::vector<ExternalRawRoom> input,
                                   std::vector<RawInfomark> marks);

public:
    NODISCARD MapPair build() &&;
    NODISCARD static MapPair buildFrom(ProgressCounter &counter,
                                       std::vector<ExternalRawRoom> rooms,
                                       std::vector<RawInfomark> marks);
};
