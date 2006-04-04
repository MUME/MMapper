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

#ifndef ABSTRACTMAPSTORAGE_H
#define ABSTRACTMAPSTORAGE_H

#include <QtGui>
#include "defs.h"

class Room;
class InfoMark;
class MapData;

class AbstractMapStorage : public QObject {

    Q_OBJECT

public:
    AbstractMapStorage(MapData&, const QString&, QFile*);
    AbstractMapStorage(MapData&, const QString&);
    ~AbstractMapStorage();

    virtual bool canLoad() = 0;
    virtual bool canSave() = 0;

    virtual void newData () = 0;
    virtual bool loadData() = 0;
    virtual bool mergeData() = 0;
    virtual bool saveData() = 0;


signals:
    void log( const QString&, const QString& );
    void onDataLoaded();
    void onDataSaved();
    void onNewData();
    void onPercentageChanged(quint32);

protected:
     QFile *m_file;
     MapData &m_mapData;
     QString m_fileName;        

private:
};


#endif
