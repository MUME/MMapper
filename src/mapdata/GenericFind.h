#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "../map/RoomIdSet.h"

class Map;
class RoomFilter;

NODISCARD RoomIdSet genericFind(const Map &map, const RoomFilter &f);
