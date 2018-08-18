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

#include "ConnectionLineBuilder.h"

#include "../mapdata/ExitDirection.h"

void ConnectionLineBuilder::drawConnLineStart(const ExitDirection dir,
                                              const bool neighbours,
                                              const double srcZ)
{
    switch (dir) {
    case ExitDirection::NORTH:
        addVertex(0.75, 0.1, srcZ);
        addVertex(0.75, -0.1, srcZ);
        break;
    case ExitDirection::SOUTH:
        addVertex(0.25, 0.9, srcZ);
        addVertex(0.25, 1.1, srcZ);
        break;
    case ExitDirection::EAST:
        addVertex(0.9, 0.25, srcZ);
        addVertex(1.1, 0.25, srcZ);
        break;
    case ExitDirection::WEST:
        addVertex(0.1, 0.75, srcZ);
        addVertex(-0.1, 0.75, srcZ);
        break;

    case ExitDirection::UP:
        drawConnStartLineUp(neighbours, srcZ);
        break;
    case ExitDirection::DOWN:
        drawConnStartLineDown(neighbours, srcZ);
        break;

    case ExitDirection::UNKNOWN:
    case ExitDirection::NONE:
        // REVISIT: Should UNKNOWN/NONE have a starting point?
    default:
        break;
    }
}

void ConnectionLineBuilder::drawConnStartLineDown(const bool neighbours, const double srcZ)
{
    if (!neighbours) {
        addVertex(0.37, 0.75, srcZ);
        addVertex(0.45, 0.75, srcZ);
    } else {
        addVertex(0.25, 0.75, srcZ);
    }
}

void ConnectionLineBuilder::drawConnStartLineUp(const bool neighbours, const double srcZ)
{
    if (!neighbours) {
        addVertex(0.63, 0.25, srcZ);
        addVertex(0.55, 0.25, srcZ);
    } else {
        addVertex(0.75, 0.25, srcZ);
    }
}

void ConnectionLineBuilder::drawConnLineEnd2Way(
    ExitDirection endDir, bool neighbours, qint32 dX, qint32 dY, double dstZ)
{
    switch (endDir) {
    case ExitDirection::NORTH:
        addVertex(dX + 0.75, dY - 0.1, dstZ);
        addVertex(dX + 0.75, dY + 0.1, dstZ);
        break;
    case ExitDirection::SOUTH:
        addVertex(dX + 0.25, dY + 1.1, dstZ);
        addVertex(dX + 0.25, dY + 0.9, dstZ);
        break;
    case ExitDirection::EAST:
        addVertex(dX + 1.1, dY + 0.25, dstZ);
        addVertex(dX + 0.9, dY + 0.25, dstZ);
        break;
    case ExitDirection::WEST:
        addVertex(dX - 0.1, dY + 0.75, dstZ);
        addVertex(dX + 0.1, dY + 0.75, dstZ);
        break;
    case ExitDirection::UP:
        drawConnEndLineUp(neighbours, dX, dY, dstZ);
        break;
    case ExitDirection::DOWN:
        drawConnEndLineDown(neighbours, dX, dY, dstZ);
        break;
    case ExitDirection::UNKNOWN:
        addVertex(dX + 0.75, dY + 0.75, dstZ);
        addVertex(dX + 0.5, dY + 0.5, dstZ);
        break;
    case ExitDirection::NONE:
        addVertex(dX + 0.75, dY + 0.75, dstZ);
        addVertex(dX + 0.5, dY + 0.5, dstZ);
        break;
    }
}

void ConnectionLineBuilder::drawConnEndLineDown(const bool neighbours,
                                                const qint32 dX,
                                                const qint32 dY,
                                                const double dstZ)
{
    if (!neighbours) {
        addVertex(dX + 0.45, dY + 0.75, dstZ);
        addVertex(dX + 0.37, dY + 0.75, dstZ);
    } else {
        addVertex(dX + 0.25, dY + 0.75, dstZ);
    }
}

void ConnectionLineBuilder::drawConnEndLineUp(const bool neighbours,
                                              const qint32 dX,
                                              const qint32 dY,
                                              const double dstZ)
{
    if (!neighbours) {
        addVertex(dX + 0.55, dY + 0.25, dstZ);
        addVertex(dX + 0.63, dY + 0.25, dstZ);
    } else {
        addVertex(dX + 0.75, dY + 0.25, dstZ);
    }
}

void ConnectionLineBuilder::drawConnLineEnd1Way(const ExitDirection endDir,
                                                const qint32 dX,
                                                const qint32 dY,
                                                const double dstZ)
{
    switch (endDir) {
    case ExitDirection::NORTH:
        addVertex(dX + 0.25, dY - 0.1, dstZ);
        addVertex(dX + 0.25, dY + 0.1, dstZ);
        break;
    case ExitDirection::SOUTH:
        addVertex(dX + 0.75, dY + 1.1, dstZ);
        addVertex(dX + 0.75, dY + 0.9, dstZ);
        break;
    case ExitDirection::EAST:
        addVertex(dX + 1.1, dY + 0.75, dstZ);
        addVertex(dX + 0.9, dY + 0.75, dstZ);
        break;
    case ExitDirection::WEST:
        addVertex(dX - 0.1, dY + 0.25, dstZ);
        addVertex(dX + 0.1, dY + 0.25, dstZ);
        break;
    case ExitDirection::UP:
    case ExitDirection::DOWN:
        addVertex(dX + 0.75, dY + 0.75, dstZ);
        addVertex(dX + 0.5, dY + 0.5, dstZ);
        break;
    case ExitDirection::UNKNOWN:
    case ExitDirection::NONE:
        // REVISIT: Original code used the UP/DOWN logic for UNKNOWN/NONE but has no starting point
        break;
    }
}
