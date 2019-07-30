#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>

#include "../mapdata/mmapper2room.h"
#include "../pandoragroup/mmapper2character.h"
#include "RoadIndex.h"

QString getResourceFilenameRaw(const QString &dir, const QString &name);
QString getPixmapFilenameRaw(const QString &name);
QString getPixmapFilename(RoomTerrainEnum);
QString getPixmapFilename(RoomLoadFlagEnum);
QString getPixmapFilename(RoomMobFlagEnum);
QString getPixmapFilename(TaggedRoad);
QString getPixmapFilename(TaggedTrail);
QString getIconFilename(CharacterPositionEnum);
QString getIconFilename(CharacterAffectEnum);
