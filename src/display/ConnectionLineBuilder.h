#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <vector>

#include "../mapdata/ExitDirection.h"
#include "../opengl/OpenGL.h"

class NODISCARD ConnectionLineBuilder final
{
private:
    std::vector<glm::vec3> &points;

public:
    explicit ConnectionLineBuilder(std::vector<glm::vec3> &points)
        : points{points}
    {}

private:
    void addVertex(float x, float y, float z) { points.emplace_back(x, y, z); }

private:
    void drawConnStartLineUp(bool neighbours, float srcZ);
    void drawConnStartLineDown(bool neighbours, float srcZ);

private:
    void drawConnEndLineUp(bool neighbours, float dX, float dY, float dstZ);
    void drawConnEndLineDown(bool neighbours, float dX, float dY, float dstZ);

public:
    void drawConnLineStart(ExitDirEnum dir, bool neighbours, float srcZ);
    void drawConnLineEnd2Way(ExitDirEnum endDir, bool neighbours, float dX, float dY, float dstZ);
    void drawConnLineEnd1Way(ExitDirEnum endDir, float dX, float dY, float dstZ);
};
