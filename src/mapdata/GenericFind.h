#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "../map/RoomIdSet.h"

class Map;
class ProgressCounter;
class RoomFilter;

// Cancelable: throws ProgressCanceledException if pc is canceled while scanning.
NODISCARD RoomIdSet genericFind(const Map &map, const RoomFilter &f, ProgressCounter &pc);
NODISCARD RoomIdSet genericFind(const Map &map, const RoomFilter &f);
