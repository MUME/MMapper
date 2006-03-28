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

#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "coordinate.h"

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
