/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef ABSTRACTMAPSTORAGE_H
#define ABSTRACTMAPSTORAGE_H

#include <QObject>
#include <QPointer>

class Room;
class InfoMark;
class MapData;
class RoomSaveFilter;
class ProgressCounter;
class QFile;
class QtIOCompressor;

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
    virtual bool saveData( bool baseMapOnly = false ) = 0;
    const ProgressCounter *progressCounter() const;

signals:
    void log( const QString&, const QString& );
    void onDataLoaded();
    void onDataSaved();
    void onNewData();

protected:
    QFile *m_file;
    QtIOCompressor *m_compressor;
    MapData &m_mapData;
    QString m_fileName;
    QPointer<ProgressCounter> m_progressCounter;
};


#endif
