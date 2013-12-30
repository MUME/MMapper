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

#ifndef FRUSTUM_H
#define FRUSTUM_H

class Coordinate;

/**
represents a viewable Frustum in the coordinate system
 
@author alve,,,
*/

// We create an enum of the sides so we don't have to call each side 0 or 1.
// This way it makes it more understandable and readable when dealing with frustum sides.
enum FrustumSide {
    F_RIGHT   = 0,        // The RIGHT side of the frustum
    F_LEFT    = 1,        // The LEFT  side of the frustum
    F_BOTTOM  = 2,        // The BOTTOM side of the frustum
    F_TOP     = 3,        // The TOP side of the frustum
    F_BACK    = 4,        // The BACK side of the frustum
    F_FRONT   = 5         // The FRONT side of the frustum
}; 

// Like above, instead of saying a number for the ABC and D of the plane, we
// want to be more descriptive.
enum PlaneData {
    A = 0,              // The X value of the plane's normal
    B = 1,              // The Y value of the plane's normal
    C = 2,              // The Z value of the plane's normal
    D = 3               // The distance the plane is from the origin
};






class Frustum
{
public:
  Frustum();

  ~Frustum();
  
  
  bool PointInFrustum(Coordinate & c);
  void rebuild(float * clip);
  float getDistance(Coordinate & c, int side = F_FRONT);
private:
  //Coordinate center;
  void NormalizePlane(int side);
  float frustum[6][4];

};

#ifdef DMALLOC
#include <mpatrol.h>
#endif

#endif
