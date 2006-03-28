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

#ifndef CONNECTIONSELECTION_H
#define CONNECTIONSELECTION_H

#include <QtGui>
#include <QtOpenGL>
#include "roomrecipient.h"
#include "room.h"
#include "exit.h"
#include "coordinate.h"
#include "mapfrontend.h"
#include "mmapper2exit.h"

class ConnectionSelection : public QObject, public RoomRecipient
{
    Q_OBJECT

public:
	
	struct ConnectionDescriptor
	{
		const Room* room;	
		ExitDirection direction;
	};

    ConnectionSelection(MapFrontend* , double mx, double my, int layer);
    ConnectionSelection();
    ~ConnectionSelection();
    
    void setFirst(MapFrontend* , double mx, double my, int layer);
	void setFirst(MapFrontend* mf, uint id, ExitDirection dir);
    void setSecond(MapFrontend* , double mx, double my, int layer);
	void setSecond(MapFrontend* mf, uint id, ExitDirection dir);
	
	void removeFirst();
	void removeSecond();
	
	ConnectionDescriptor getFirst();
	ConnectionDescriptor getSecond();

	bool isValid();
	bool isFirstValid() { if(m_connectionDescriptor[0].room == NULL) return false; else return true; };
	bool isSecondValid(){ if(m_connectionDescriptor[1].room == NULL) return false; else return true; };
	
	void receiveRoom(RoomAdmin * admin, const Room * aRoom);
    
public slots:

signals:

protected:
	
private:

	ExitDirection ComputeDirection(double mouseX, double mouseY);

	int GLtoMap(double arg);

	ConnectionDescriptor m_connectionDescriptor[2];

	bool m_first;
	RoomAdmin * m_admin;
};


#endif
