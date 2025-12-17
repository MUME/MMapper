#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../clock/mumemoment.h"
#include "../group/mmapper2character.h"
#include "../map/mmapper2room.h"
#include "RoadIndex.h"

#include <QString>

// Season management for texture loading
extern void setCurrentSeason(MumeSeasonEnum season);
NODISCARD extern MumeSeasonEnum getCurrentSeason();

NODISCARD extern QString getResourceFilenameRaw(const QString &dir, const QString &name);
NODISCARD extern QString getPixmapFilenameRaw(const QString &name);
NODISCARD extern QString getPixmapFilename(RoomTerrainEnum);
NODISCARD extern QString getPixmapFilename(RoomLoadFlagEnum);
NODISCARD extern QString getPixmapFilename(RoomMobFlagEnum);
NODISCARD extern QString getPixmapFilename(TaggedRoad);
NODISCARD extern QString getPixmapFilename(TaggedTrail);
NODISCARD extern QString getIconFilename(CharacterPositionEnum);
NODISCARD extern QString getIconFilename(CharacterAffectEnum);
