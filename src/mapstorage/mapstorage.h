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

#ifndef MAPSTORAGE_H
#define MAPSTORAGE_H

#include <QtGui>
#include "defs.h"
#include "abstractmapstorage.h"
#include "oldconnection.h"
#include "oldroom.h"
#include "roomfactory.h"

class MapStorage : public AbstractMapStorage {

    Q_OBJECT

public:
    MapStorage(MapData&, const QString&, QFile*);
    MapStorage(MapData&, const QString&);

    virtual bool canLoad() {return TRUE;};
    virtual bool canSave() {return TRUE;};

    virtual void newData ();
    virtual bool loadData();
    virtual bool saveData();
    bool mergeData();
    
private:
    RoomFactory factory;
    Room * loadRoom(QDataStream & stream, qint32 version);
    void loadExits(Room * room, QDataStream & stream, qint32 version);
    Room * loadOldRoom(QDataStream & stream, ConnectionList & connectionList);
    void loadOldConnection(Connection *, QDataStream & stream, RoomList & roomList);
    void loadMark(InfoMark * mark, QDataStream & stream, qint32 version);
    void saveMark(InfoMark * mark, QDataStream & stream);
    void translateOldConnection(Connection *);
    void saveRoom(const Room * room, QDataStream & stream);
    void saveExits(const Room * room, QDataStream & stream);
    
    uint baseId;
    Coordinate basePosition;
};

class MapFrontendBlocker {
  public:
    MapFrontendBlocker(MapFrontend & in_data) : data(in_data) {data.block();}
    ~MapFrontendBlocker() {data.unblock();}
  private:
    MapFrontend & data;
};


#endif
