// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ConnectionLineBuilder.h"

#include <cassert>

#include "../mapdata/ExitDirection.h"

void ConnectionLineBuilder::drawConnLineStart(const ExitDirEnum dir,
                                              const bool neighbours,
                                              const float srcZ)
{
    switch (dir) {
    case ExitDirEnum::NORTH:
        addVertex(0.75f, 0.9f, srcZ);
        addVertex(0.75f, 1.1f, srcZ);
        break;
    case ExitDirEnum::SOUTH:
        addVertex(0.25f, 0.1f, srcZ);
        addVertex(0.25f, -0.1f, srcZ);
        break;
    case ExitDirEnum::EAST:
        addVertex(0.9f, 0.75f, srcZ);
        addVertex(1.1f, 0.75f, srcZ);
        break;
    case ExitDirEnum::WEST:
        addVertex(0.1f, 0.25f, srcZ);
        addVertex(-0.1f, 0.25f, srcZ);
        break;

    case ExitDirEnum::UP:
        drawConnStartLineUp(neighbours, srcZ);
        break;
    case ExitDirEnum::DOWN:
        drawConnStartLineDown(neighbours, srcZ);
        break;

    case ExitDirEnum::UNKNOWN:
        addVertex(0.5f, 0.5f, srcZ);
        addVertex(0.75f, 0.25f, srcZ);
        break;

    case ExitDirEnum::NONE:
        assert(false);
        break;

    default:
        break;
    }
}

void ConnectionLineBuilder::drawConnStartLineDown(const bool neighbours, const float srcZ)
{
    if (!neighbours) {
        addVertex(0.37f, 0.25f, srcZ);
        addVertex(0.45f, 0.25f, srcZ);
    } else {
        addVertex(0.25f, 0.25f, srcZ);
    }
}

void ConnectionLineBuilder::drawConnStartLineUp(const bool neighbours, const float srcZ)
{
    if (!neighbours) {
        addVertex(0.63f, 0.75f, srcZ);
        addVertex(0.55f, 0.75f, srcZ);
    } else {
        addVertex(0.75f, 0.75f, srcZ);
    }
}

void ConnectionLineBuilder::drawConnLineEnd2Way(const ExitDirEnum endDir,
                                                const bool neighbours,
                                                const float dX,
                                                const float dY,
                                                const float dstZ)
{
    switch (endDir) {
    case ExitDirEnum::NORTH:
        addVertex(dX + 0.75f, dY + 1.1f, dstZ);
        addVertex(dX + 0.75f, dY + 0.9f, dstZ);
        break;
    case ExitDirEnum::SOUTH:
        addVertex(dX + 0.25f, dY - 0.1f, dstZ);
        addVertex(dX + 0.25f, dY + 0.1f, dstZ);
        break;
    case ExitDirEnum::EAST:
        addVertex(dX + 1.1f, dY + 0.75f, dstZ);
        addVertex(dX + 0.9f, dY + 0.75f, dstZ);
        break;
    case ExitDirEnum::WEST:
        addVertex(dX - 0.1f, dY + 0.25f, dstZ);
        addVertex(dX + 0.1f, dY + 0.25f, dstZ);
        break;
    case ExitDirEnum::UP:
        drawConnEndLineUp(neighbours, dX, dY, dstZ);
        break;
    case ExitDirEnum::DOWN:
        drawConnEndLineDown(neighbours, dX, dY, dstZ);
        break;
    case ExitDirEnum::UNKNOWN:
        addVertex(dX + 0.75f, dY + 0.25f, dstZ);
        addVertex(dX + 0.5f, dY + 0.5f, dstZ);
        break;
    case ExitDirEnum::NONE:
        // REVISIT: This logic used to match UNKNOWN but it does not make sense for
        // Rooms to be connected via NONE
        assert(false);
        break;
    }
}

void ConnectionLineBuilder::drawConnEndLineDown(const bool neighbours,
                                                const float dX,
                                                const float dY,
                                                const float dstZ)
{
    if (!neighbours) {
        addVertex(dX + 0.45f, dY + 0.25f, dstZ);
        addVertex(dX + 0.37f, dY + 0.25f, dstZ);
    } else {
        addVertex(dX + 0.25f, dY + 0.25f, dstZ);
    }
}

void ConnectionLineBuilder::drawConnEndLineUp(const bool neighbours,
                                              const float dX,
                                              const float dY,
                                              const float dstZ)
{
    if (!neighbours) {
        addVertex(dX + 0.55f, dY + 0.75f, dstZ);
        addVertex(dX + 0.63f, dY + 0.75f, dstZ);
    } else {
        addVertex(dX + 0.75f, dY + 0.75f, dstZ);
    }
}

void ConnectionLineBuilder::drawConnLineEnd1Way(const ExitDirEnum endDir,
                                                const float dX,
                                                const float dY,
                                                const float dstZ)
{
    switch (endDir) {
    case ExitDirEnum::NORTH:
        addVertex(dX + 0.25f, dY + 1.1f, dstZ);
        addVertex(dX + 0.25f, dY + 0.9f, dstZ);
        break;
    case ExitDirEnum::SOUTH:
        addVertex(dX + 0.75f, dY - 0.1f, dstZ);
        addVertex(dX + 0.75f, dY + 0.1f, dstZ);
        break;
    case ExitDirEnum::EAST:
        addVertex(dX + 1.1f, dY + 0.25f, dstZ);
        addVertex(dX + 0.9f, dY + 0.25f, dstZ);
        break;
    case ExitDirEnum::WEST:
        addVertex(dX - 0.1f, dY + 0.75f, dstZ);
        addVertex(dX + 0.1f, dY + 0.75f, dstZ);
        break;
    case ExitDirEnum::UP:
    case ExitDirEnum::DOWN:
        addVertex(dX + 0.75f, dY + 0.25f, dstZ);
        addVertex(dX + 0.5f, dY + 0.5f, dstZ);
        break;
    case ExitDirEnum::UNKNOWN:
        addVertex(dX + 0.75f, dY + 0.25f, dstZ);
        addVertex(dX + 0.5f, dY + 0.5f, dstZ);
        break;
    case ExitDirEnum::NONE:
        assert(false);
        break;
    }
}
