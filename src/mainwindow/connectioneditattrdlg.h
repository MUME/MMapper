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

#ifndef CONNECTIONEDITATTRDLG_H
#define CONNECTIONEDITATTRDLG_H

#include <QDialog>
#include "ui_connectioneditattrdlg.h"
#include "connection.h"

class MapData;
class Room;
class Connection;
class ConnectionSelection;

class ConnectionEditAttrDlg : public QDialog, private Ui::ConnectionEditAttrDlg
{
    Q_OBJECT

public slots:
    void setConnectionSelection(ConnectionSelection*, MapData*);   
    
    void applyClicked();
	void closeClicked();
	

public:
    ConnectionEditAttrDlg(QWidget *parent = 0);
    
private:
	void updateDialog();
	Connection* searchForConnection(Room* r1, Room* r2, ConnectionDirection dir1, ConnectionDirection dir2);

	ConnectionSelection* m_connectionSelection;
	MapData* m_mapData;
};


#endif
