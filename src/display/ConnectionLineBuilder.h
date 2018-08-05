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

#ifndef MMAPPER_CONNECTION_LINE_BUILDER_H
#define MMAPPER_CONNECTION_LINE_BUILDER_H

#include <vector>

#include "../mapdata/ExitDirection.h"
#include "OpenGL.h"

struct Vec3d;

class ConnectionLineBuilder
{
private:
    std::vector<Vec3d> &points;

public:
    explicit ConnectionLineBuilder(std::vector<Vec3d> &points)
        : points{points}
    {}

private:
    void addVertex(double x, double y, double z) { points.emplace_back(x, y, z); }

private:
    void drawConnStartLineUp(bool neighbours, double srcZ);
    void drawConnStartLineDown(bool neighbours, double srcZ);

private:
    void drawConnEndLineUp(bool neighbours, qint32 dX, qint32 dY, double dstZ);
    void drawConnEndLineDown(bool neighbours, qint32 dX, qint32 dY, double dstZ);

public:
    void drawConnLineStart(ExitDirection dir, bool neighbours, double srcZ);
    void drawConnLineEnd2Way(
        ExitDirection endDir, bool neighbours, qint32 dX, qint32 dY, double dstZ);
    void drawConnLineEnd1Way(ExitDirection endDir, qint32 dX, qint32 dY, double dstZ);
};

#endif // MMAPPER_CONNECTION_LINE_BUILDER_H
