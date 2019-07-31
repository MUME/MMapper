#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <vector>

#include "../mapdata/ExitDirection.h"
#include "OpenGL.h"

struct Vec3f;

class ConnectionLineBuilder
{
private:
    std::vector<Vec3f> &points;

public:
    explicit ConnectionLineBuilder(std::vector<Vec3f> &points)
        : points{points}
    {}

private:
    void addVertex(float x, float y, float z) { points.emplace_back(x, y, z); }

private:
    void drawConnStartLineUp(bool neighbours, float srcZ);
    void drawConnStartLineDown(bool neighbours, float srcZ);

private:
    void drawConnEndLineUp(bool neighbours, qint32 dX, qint32 dY, float dstZ);
    void drawConnEndLineDown(bool neighbours, qint32 dX, qint32 dY, float dstZ);

public:
    void drawConnLineStart(ExitDirEnum dir, bool neighbours, float srcZ);
    void drawConnLineEnd2Way(ExitDirEnum endDir, bool neighbours, qint32 dX, qint32 dY, float dstZ);
    void drawConnLineEnd1Way(ExitDirEnum endDir, qint32 dX, qint32 dY, float dstZ);
};
