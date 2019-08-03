#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <map>
#include <memory>
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
#include "../parser/CommandQueue.h"
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
class RoomFieldVariant;
class RoomFilter;
class RoomRecipient;
class ShortestPathRecipient;

using ConstRoomList = QList<const Room *>;
using MarkerList = std::list<std::shared_ptr<InfoMark>>;

class MapData final : public MapFrontend
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

    void draw(const Coordinate &min, const Coordinate &max, MapCanvasRoomDrawer &screen);

    bool execute(std::unique_ptr<MapAction> action, const SharedRoomSelection &unlock);

    Coordinate &getPosition() { return m_position; }
    MarkerList &getMarkersList() { return m_markers; }
    uint getRoomsCount() const
    {
        return (greatestUsedId == INVALID_ROOMID) ? 0u : (greatestUsedId.asUint32() + 1u);
    }

    void addMarker(InfoMark *im);
    void removeMarker(InfoMark *im);
    void addMarker(const std::shared_ptr<InfoMark> &im);
    void removeMarker(const std::shared_ptr<InfoMark> &im);

    bool isEmpty() const { return (greatestUsedId == INVALID_ROOMID) && m_markers.empty(); }
    bool dataChanged() const { return m_dataChanged; }
    QList<Coordinate> getPath(const Coordinate &start, const CommandQueue &dirs);
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
    QString getDoorName(const Coordinate &pos, ExitDirEnum dir);
    QString getDoorName(const Coordinate &pos, DirectionEnum dir)
    {
        return getDoorName(pos, static_cast<ExitDirEnum>(dir));
    }
    void setDoorName(const Coordinate &pos, const QString &name, ExitDirEnum dir);

public:
    void setFileName(QString filename, bool readOnly)
    {
        m_fileName = filename;
        m_fileReadOnly = readOnly;
    }
    const QString &getFileName() const { return m_fileName; }
    bool isFileReadOnly() const { return m_fileReadOnly; }

public:
    bool getExitFlag(const Coordinate &pos, ExitDirEnum dir, ExitFieldVariant var);
    void toggleExitFlag(const Coordinate &pos, ExitDirEnum dir, ExitFieldVariant var);

public:
    bool getRoomFlag(const Coordinate &pos, RoomFieldVariant var);
    void toggleRoomFlag(const Coordinate &pos, RoomFieldVariant var);

signals:
    void log(const QString &, const QString &);
    void onDataLoaded();
    void onDataChanged();

public slots:
    void unsetDataChanged() { m_dataChanged = false; }
    void setDataChanged() { m_dataChanged = true; }
    void setPosition(const Coordinate &pos) { m_position = pos; }
    void slot_scheduleAction(std::shared_ptr<MapAction> action)
    {
        MapFrontend::scheduleAction(action);
    }

protected:
    MarkerList m_markers;
    // changed data?
    bool m_dataChanged = false;
    bool m_fileReadOnly = false;
    QString m_fileName;
    Coordinate m_position;
};
