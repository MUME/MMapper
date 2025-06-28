#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "../map/Map.h"
#include "../map/infomark.h"

#include <vector>

struct NODISCARD RawMapLoadData final
{
    std::vector<ExternalRawRoom> rooms;
    std::vector<RawInfomark> markers;
    Coordinate position;
    QString filename;
    bool readonly = true;
};

struct NODISCARD MapLoadData final
{
    MapPair mapPair;
    Coordinate position;
    QString filename;
    bool readonly = true;
};
