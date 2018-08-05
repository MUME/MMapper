/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#include "frustum.h"
#include "../global/EnumIndexedArray.h"
#include "coordinate.h"
#include <cmath>

void Frustum::normalizePlane(const FrustumSide side)
{
    // Here we calculate the magnitude of the normal to the plane (point A B C)
    // Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
    // To calculate magnitude you use the equation:  magnitude = sqrt( x^2 + y^2 + z^2)
    auto &plane = frustum[side];
    auto magnitude = std::sqrt(plane[PlaneData::A] * plane[PlaneData::A]
                               + plane[PlaneData::B] * plane[PlaneData::B]
                               + plane[PlaneData::C] * plane[PlaneData::C]);

    // Then we divide the plane's values by it's magnitude.
    // This makes it easier to work with.
    plane[PlaneData::A] /= magnitude;
    plane[PlaneData::B] /= magnitude;
    plane[PlaneData::C] /= magnitude;
    plane[PlaneData::D] /= magnitude;
}

bool Frustum::pointInFrustum(const Coordinate &c) const
{
    // Go through all the sides of the frustum
    for (const Plane &plane : frustum) {
        // Calculate the plane equation and check if the point is behind a side of the frustum
        // REVISIT: Could rewrite this to use getDistance().
        if (plane[PlaneData::A] * static_cast<float>(c.x)
                + plane[PlaneData::B] * static_cast<float>(c.y)
                + plane[PlaneData::C] * static_cast<float>(c.z) + plane[PlaneData::D]
            <= 0) {
            // The point was behind a side, so it ISN'T in the frustum
            return false;
        }
    }

    // The point was inside of the frustum (In front of ALL the sides of the frustum)
    return true;
}

void Frustum::rebuild(const float *const clip)
{
    // Now we actually want to get the sides of the frustum.  To do this we take
    // the clipping planes we received above and extract the sides from them.

    // This will extract the RIGHT side of the frustum
    auto &right = frustum[FrustumSide::F_RIGHT];
    right[PlaneData::A] = clip[3] - clip[0];
    right[PlaneData::B] = clip[7] - clip[4];
    right[PlaneData::C] = clip[11] - clip[8];
    right[PlaneData::D] = clip[15] - clip[12];

    // Now that we have a normal (A,B,C) and a distance (D) to the plane,
    // we want to normalize that normal and distance.

    // Normalize the RIGHT side
    normalizePlane(FrustumSide::F_RIGHT);

    // This will extract the LEFT side of the frustum
    auto &left = frustum[FrustumSide::F_LEFT];
    left[PlaneData::A] = clip[3] + clip[0];
    left[PlaneData::B] = clip[7] + clip[4];
    left[PlaneData::C] = clip[11] + clip[8];
    left[PlaneData::D] = clip[15] + clip[12];

    // Normalize the LEFT side
    normalizePlane(FrustumSide::F_LEFT);

    // This will extract the BOTTOM side of the frustum
    auto &bottom = frustum[FrustumSide::F_BOTTOM];
    bottom[PlaneData::A] = clip[3] + clip[1];
    bottom[PlaneData::B] = clip[7] + clip[5];
    bottom[PlaneData::C] = clip[11] + clip[9];
    bottom[PlaneData::D] = clip[15] + clip[13];

    // Normalize the BOTTOM side
    normalizePlane(FrustumSide::F_BOTTOM);

    // This will extract the TOP side of the frustum
    auto &top = frustum[FrustumSide::F_TOP];
    top[PlaneData::A] = clip[3] - clip[1];
    top[PlaneData::B] = clip[7] - clip[5];
    top[PlaneData::C] = clip[11] - clip[9];
    top[PlaneData::D] = clip[15] - clip[13];

    // Normalize the TOP side
    normalizePlane(FrustumSide::F_TOP);

    // This will extract the BACK side of the frustum
    auto &back = frustum[FrustumSide::F_BACK];
    back[PlaneData::A] = clip[3] - clip[2];
    back[PlaneData::B] = clip[7] - clip[6];
    back[PlaneData::C] = clip[11] - clip[10];
    back[PlaneData::D] = clip[15] - clip[14];

    // Normalize the BACK side
    normalizePlane(FrustumSide::F_BACK);

    // This will extract the FRONT side of the frustum
    auto &front = frustum[FrustumSide::F_FRONT];
    front[PlaneData::A] = clip[3] + clip[2];
    front[PlaneData::B] = clip[7] + clip[6];
    front[PlaneData::C] = clip[11] + clip[10];
    front[PlaneData::D] = clip[15] + clip[14];

    // Normalize the FRONT side
    normalizePlane(FrustumSide::F_FRONT);
}

float Frustum::getDistance(const Coordinate &c, FrustumSide side) const
{
    const auto &plane = frustum[side];
    return plane[PlaneData::A] * static_cast<float>(c.x)
           + plane[PlaneData::B] * static_cast<float>(c.y)
           + plane[PlaneData::C] * static_cast<float>(c.z) + plane[PlaneData::D];
}
