#pragma once
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

#ifndef MAPDATA_H
#define MAPDATA_H

#include <map>
#include <QLinkedList>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QtCore>
#include <QtGlobal>

#include "../display/OpenGL.h"
#include "../expandoracommon/coordinate.h"
#include "../global/DirectionType.h"
#include "../global/roomid.h"
#include "../mapfrontend/mapfrontend.h"
#include "../parser/CommandId.h"
#include "../parser/abstractparser.h"
#include "ExitDirection.h"
#include "mmapper2exit.h"
#include "mmapper2room.h"
#include "roomfilter.h"
#include "roomselection.h"
#include "shortestpath.h"

class AbstractAction;
class ExitFieldVariant;
class InfoMark;
class MapAction;
class MapCanvasRoomDrawer;
class QObject;
class Room;
class RoomFilter;
class RoomRecipient;
class ShortestPathRecipient;

using ConstRoomList = QList<const Room *>;
using RoomVector = QVector<Room *>;
using MarkerList = QLinkedList<InfoMark *>;
using MarkerListIterator = QLinkedListIterator<InfoMark *>;

class MapData : public MapFrontend
{
    Q_OBJECT
    friend class RoomSelection;

protected:
    // the room will be inserted in the given selection. the selection must have been created by mapdata
    const Room *getRoom(const Coordinate &pos, RoomSelection &in);
    const Room *getRoom(RoomId id, RoomSelection &in);

public:
    explicit MapData(QObject *parent = nullptr);
    virtual ~MapData() override;

    void draw(const Coordinate &ulf, const Coordinate &lrb, MapCanvasRoomDrawer &screen);

    bool execute(MapAction *action, const SharedRoomSelection &unlock);

    Coordinate &getPosition() { return m_position; }
    MarkerList &getMarkersList() { return m_markers; }
    uint getRoomsCount() const
    {
        return (greatestUsedId == INVALID_ROOMID) ? 0u : (greatestUsedId.asUint32() + 1u);
    }

    void addMarker(InfoMark *im);
    void removeMarker(InfoMark *im);

    bool isEmpty() const { return (greatestUsedId == INVALID_ROOMID) && m_markers.isEmpty(); }
    bool dataChanged() const { return m_dataChanged; }
    const QString &getFileName() const { return m_fileName; }
    QList<Coordinate> getPath(const QList<CommandIdType> &dirs);
    virtual void clear() override;

    // search for matches
    void genericSearch(RoomRecipient *recipient, const RoomFilter &f);

    void shortestPathSearch(const Room *origin,
                            ShortestPathRecipient *recipient,
                            const RoomFilter &f,
                            int max_hits = -1,
                            double max_dist = 0);

    // Used in Console Commands
    void removeDoorNames();
    QString getDoorName(const Coordinate &pos, ExitDirection dir);
    QString getDoorName(const Coordinate &pos, DirectionType dir)
    {
        return getDoorName(pos, static_cast<ExitDirection>(dir));
    }
    void setDoorName(const Coordinate &pos, const QString &name, ExitDirection dir);

public:
    bool getExitFlag(const Coordinate &pos, ExitDirection dir, ExitFieldVariant var);
    void toggleExitFlag(const Coordinate &pos, ExitDirection dir, ExitFieldVariant var);

public:
    [[deprecated]] void setRoomField(const Coordinate &pos, const QVariant &flag, RoomField field);
    [[deprecated]] QVariant getRoomField(const Coordinate &pos, RoomField field);
    [[deprecated]] void toggleRoomFlag(const Coordinate &pos, uint flag, RoomField field);
    [[deprecated]] bool getRoomFlag(const Coordinate &pos, uint flag, RoomField field);

signals:
    void log(const QString &, const QString &);
    void onDataLoaded();
    void onDataChanged();
    void updateCanvas();

public slots:
    void setFileName(QString filename) { m_fileName = filename; }
    void unsetDataChanged() { m_dataChanged = false; }
    void setDataChanged() { m_dataChanged = true; }
    void setPosition(const Coordinate &pos) { m_position = pos; }

protected:
    MarkerList m_markers{};
    // changed data?
    bool m_dataChanged = false;
    QString m_fileName{};
    Coordinate m_position{};
};

#endif
