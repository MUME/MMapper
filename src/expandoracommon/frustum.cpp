/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#include "frustum.h"
#include <cmath>

Frustum::Frustum()
{}


Frustum::~Frustum()
{}




void Frustum::NormalizePlane(int side)
{
  // Here we calculate the magnitude of the normal to the plane (point A B C)
  // Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
  // To calculate magnitude you use the equation:  magnitude = sqrt( x^2 + y^2 + z^2)
  float magnitude = (float)sqrt( frustum[side][A] * frustum[side][A] +
                                 frustum[side][B] * frustum[side][B] +
                                 frustum[side][C] * frustum[side][C] );

  // Then we divide the plane's values by it's magnitude.
  // This makes it easier to work with.
  frustum[side][A] /= magnitude;
  frustum[side][B] /= magnitude;
  frustum[side][C] /= magnitude;
  frustum[side][D] /= magnitude;
}

bool Frustum::PointInFrustum(Coordinate & c)
{
  // Go through all the sides of the frustum
  for(int i = 0; i < 6; ++i )
  {
    // Calculate the plane equation and check if the point is behind a side of the frustum
    if(frustum[i][A] * (float)c.x + frustum[i][B] * (float)c.y + frustum[i][C] * (float)c.z + frustum[i][D] <= 0)
    {
      // The point was behind a side, so it ISN'T in the frustum
      return false;
    }
  }

  // The point was inside of the frustum (In front of ALL the sides of the frustum)
  return true;
}



void Frustum::rebuild(float * clip)
{
  
  // Now we actually want to get the sides of the frustum.  To do this we take
  // the clipping planes we received above and extract the sides from them.

  // This will extract the RIGHT side of the frustum
  frustum[F_RIGHT][A] = clip[ 3] - clip[ 0];
  frustum[F_RIGHT][B] = clip[ 7] - clip[ 4];
  frustum[F_RIGHT][C] = clip[11] - clip[ 8];
  frustum[F_RIGHT][D] = clip[15] - clip[12];

  // Now that we have a normal (A,B,C) and a distance (D) to the plane,
  // we want to normalize that normal and distance.

  // Normalize the RIGHT side
  NormalizePlane(F_RIGHT);

  // This will extract the LEFT side of the frustum
  frustum[F_LEFT][A] = clip[ 3] + clip[ 0];
  frustum[F_LEFT][B] = clip[ 7] + clip[ 4];
  frustum[F_LEFT][C] = clip[11] + clip[ 8];
  frustum[F_LEFT][D] = clip[15] + clip[12];

  // Normalize the LEFT side
  NormalizePlane(F_LEFT);

  // This will extract the BOTTOM side of the frustum
  frustum[F_BOTTOM][A] = clip[ 3] + clip[ 1];
  frustum[F_BOTTOM][B] = clip[ 7] + clip[ 5];
  frustum[F_BOTTOM][C] = clip[11] + clip[ 9];
  frustum[F_BOTTOM][D] = clip[15] + clip[13];

  // Normalize the BOTTOM side
  NormalizePlane(F_BOTTOM);

  // This will extract the TOP side of the frustum
  frustum[F_TOP][A] = clip[ 3] - clip[ 1];
  frustum[F_TOP][B] = clip[ 7] - clip[ 5];
  frustum[F_TOP][C] = clip[11] - clip[ 9];
  frustum[F_TOP][D] = clip[15] - clip[13];

  // Normalize the TOP side
  NormalizePlane(F_TOP);

  // This will extract the BACK side of the frustum
  frustum[F_BACK][A] = clip[ 3] - clip[ 2];
  frustum[F_BACK][B] = clip[ 7] - clip[ 6];
  frustum[F_BACK][C] = clip[11] - clip[10];
  frustum[F_BACK][D] = clip[15] - clip[14];

  // Normalize the BACK side
  NormalizePlane(F_BACK);

  // This will extract the FRONT side of the frustum
  frustum[F_FRONT][A] = clip[ 3] + clip[ 2];
  frustum[F_FRONT][B] = clip[ 7] + clip[ 6];
  frustum[F_FRONT][C] = clip[11] + clip[10];
  frustum[F_FRONT][D] = clip[15] + clip[14];

  // Normalize the FRONT side
  NormalizePlane(F_FRONT);


}


float Frustum::getDistance(Coordinate & c, int side)
{
  return frustum[side][A] * c.x + frustum[side][B] * c.y + frustum[side][C] * c.z + frustum[side][D];
}
