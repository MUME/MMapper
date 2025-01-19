#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/mmapper2room.h"
#include "../pandoragroup/mmapper2character.h"
#include "RoadIndex.h"

#include <QString>

NODISCARD extern QString getResourceFilenameRaw(const QString &dir, const QString &name);
NODISCARD extern QString getPixmapFilenameRaw(const QString &name);
NODISCARD extern QString getPixmapFilename(RoomTerrainEnum);
NODISCARD extern QString getPixmapFilename(RoomLoadFlagEnum);
NODISCARD extern QString getPixmapFilename(RoomMobFlagEnum);
NODISCARD extern QString getPixmapFilename(TaggedRoad);
NODISCARD extern QString getPixmapFilename(TaggedTrail);
NODISCARD extern QString getIconFilename(CharacterPositionEnum);
NODISCARD extern QString getIconFilename(CharacterAffectEnum);
