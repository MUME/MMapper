#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

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
    void drawConnLineStart(ExitDirection dir, bool neighbours, float srcZ);
    void drawConnLineEnd2Way(ExitDirection endDir, bool neighbours, qint32 dX, qint32 dY, float dstZ);
    void drawConnLineEnd1Way(ExitDirection endDir, qint32 dX, qint32 dY, float dstZ);
};
