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

#include "connectionselection.h"
#include "mapdata.h"
#include "coordinate.h"

ConnectionSelection::ConnectionSelection(MapFrontend* mf, double mx, double my, int layer)
{
	m_admin = NULL;
	m_first = true;
	
	m_connectionDescriptor[0].room = NULL;
	m_connectionDescriptor[1].room = NULL;
 	
 	Coordinate c(GLtoMap(mx), GLtoMap(my), layer);
 	mf->lookingForRooms(this, c, c );
 	m_connectionDescriptor[0].direction = ComputeDirection(mx, my);
}

int ConnectionSelection::GLtoMap(double arg)
{
	if (arg >=0)
		 return (int)(arg+0.5f);
	 return (int)(arg-0.5f);	
}

ConnectionSelection::ConnectionSelection()
{
	m_admin = NULL;
	m_first = true;
	
	m_connectionDescriptor[0].room = NULL;
	m_connectionDescriptor[1].room = NULL;
}

ConnectionSelection::~ConnectionSelection()
{
	//for all rooms remove them internaly and call release room
	if (m_admin) 
	{	
		if (m_connectionDescriptor[0].room) m_admin->releaseRoom(this, m_connectionDescriptor[0].room->getId());
		if (m_connectionDescriptor[1].room) m_admin->releaseRoom(this, m_connectionDescriptor[1].room->getId());
	    m_admin = NULL;
	}
}

bool ConnectionSelection::isValid()
{
	if (m_connectionDescriptor[0].room == NULL || m_connectionDescriptor[1].room == NULL)
		return false;	
	return true;
}

ExitDirection ConnectionSelection::ComputeDirection(double mouseX, double mouseY)
{
	ExitDirection dir = ED_UNKNOWN;
	
	//room centre
   	//int x1 = (int) (mouseX + 0.5);
	//int y1 = (int) (mouseY + 0.5);
   	int x1 = GLtoMap(mouseX);
	int y1 = GLtoMap(mouseY);

   	double x1d = mouseX - x1;
   	double y1d = mouseY - y1;
	
	if ( y1d > -0.2 && y1d < 0.2)
	{
		//y1p = y1;
		if (x1d >= 0.2)
		{
			dir = ED_EAST;
			//x1p = x1 + 0.4;
		}
		else
		{
			if (x1d <= -0.2)
			{
				dir = ED_WEST;
				//x1p = x1 - 0.4;
			}
			else
			{
				dir = ED_UNKNOWN;
				//x1p = x1;				
			}
		}
	}
	else
	{
		//x1p = x1;
		if (y1d >= 0.2)
		{
			//y1p = y1 + 0.4;
			dir = ED_SOUTH;
			if (x1d <= -0.2)
			{
			  dir = ED_DOWN;
			  //x1p = x1 + 0.4;	
			}
		}
		else
		{
			if (y1d <= -0.2)
			{
				//y1p = y1 - 0.4;
				dir = ED_NORTH;
				if (x1d >= 0.2)
				{
				  dir = ED_UP;
				  //x1p = x1 - 0.4;	
				}
			}
		}
	}    
	
	return dir;
}

void ConnectionSelection::setFirst(MapFrontend* mf, double mx, double my, int layer)
{
	m_first = true;
 	Coordinate c(GLtoMap(mx), GLtoMap(my), layer);
 	if (m_connectionDescriptor[0].room != NULL)
  	{
  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  			m_admin->releaseRoom(this, m_connectionDescriptor[0].room->getId());
		m_connectionDescriptor[0].room = NULL;
  	}
 	mf->lookingForRooms(this, c, c );
	m_connectionDescriptor[0].direction = ComputeDirection(mx, my);	
	//if (m_connectionDescriptor[0].direction == CD_NONE) m_connectionDescriptor[0].direction = CD_UNKNOWN;	
}

void ConnectionSelection::setFirst(MapFrontend* mf, uint id, ExitDirection dir)
{
	m_first = true;
 	if (m_connectionDescriptor[0].room != NULL)
  	{
  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  			m_admin->releaseRoom(this, m_connectionDescriptor[0].room->getId());
		m_connectionDescriptor[0].room = NULL;
  	}
 	mf->lookingForRooms(this, id );
	m_connectionDescriptor[0].direction = dir;	
	//if (m_connectionDescriptor[0].direction == CD_NONE) m_connectionDescriptor[0].direction = CD_UNKNOWN;	
}

void ConnectionSelection::setSecond(MapFrontend* mf, double mx, double my, int layer)
{
	m_first = false;
 	Coordinate c(GLtoMap(mx), GLtoMap(my), layer);
 	if (m_connectionDescriptor[1].room != NULL)
  	{
  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  			m_admin->releaseRoom(this, m_connectionDescriptor[1].room->getId());
		m_connectionDescriptor[1].room = NULL;
  	}
 	mf->lookingForRooms(this, c, c );
	m_connectionDescriptor[1].direction = ComputeDirection(mx, my);	
	//if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;	
}

void ConnectionSelection::setSecond(MapFrontend* mf, uint id, ExitDirection dir)
{
	m_first = false;
 	if (m_connectionDescriptor[1].room != NULL)
  	{
  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  			m_admin->releaseRoom(this, m_connectionDescriptor[1].room->getId());
		m_connectionDescriptor[1].room = NULL;
  	}
 	mf->lookingForRooms(this, id );
	m_connectionDescriptor[1].direction = dir;	
	//if (m_connectionDescriptor[1].direction == ED_UNKNOWN) m_connectionDescriptor[1].direction = ED_NONE;	
}

void ConnectionSelection::removeFirst()
{
 	if (m_connectionDescriptor[0].room != NULL)
  	{
  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  			m_admin->releaseRoom(this, m_connectionDescriptor[0].room->getId());
		m_connectionDescriptor[0].room = NULL;
  	}	
}

void ConnectionSelection::removeSecond()
{
 	if (m_connectionDescriptor[1].room != NULL)
  	{
  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  			m_admin->releaseRoom(this, m_connectionDescriptor[1].room->getId());
		m_connectionDescriptor[1].room = NULL;
  	}	
}


ConnectionSelection::ConnectionDescriptor ConnectionSelection::getFirst()
{
	return m_connectionDescriptor[0];
}

ConnectionSelection::ConnectionDescriptor ConnectionSelection::getSecond()
{
	return m_connectionDescriptor[1];		
}

void ConnectionSelection::receiveRoom(RoomAdmin * admin, const Room * aRoom) 
{
  m_admin = admin;  
  //addroom to internal map
  //quint32 id = aRoom->getId();

  if (m_first)
  {
	  	if (m_connectionDescriptor[0].room != NULL)
	  	{
	  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  				m_admin->releaseRoom(this, m_connectionDescriptor[0].room->getId());
  			m_connectionDescriptor[0].room = aRoom;
	  	}
	  	else
	  	{
	  		m_connectionDescriptor[0].room = aRoom;
	  	}
  }
  else
  {
	  	if (m_connectionDescriptor[1].room != NULL)
	  	{
	  		if (m_connectionDescriptor[1].room != m_connectionDescriptor[0].room)
  				m_admin->releaseRoom(this, m_connectionDescriptor[1].room->getId());
  			m_connectionDescriptor[1].room = aRoom;
	  	}
	  	else
	  	{
	  		m_connectionDescriptor[1].room = aRoom;
	  	}
  }
}
