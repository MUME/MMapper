#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/ExitDirection.h"
#include "../map/coordinate.h"
#include "../map/roomid.h"
#include "../mapfrontend/mapfrontend.h"
#include "../parser/CommandQueue.h"
#include "roomfilter.h"
#include "roomselection.h"
#include "shortestpath.h"

#include <map>
#include <memory>
#include <vector>

#include <QList>
#include <QString>
#include <QtCore>
#include <QtGlobal>

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

using ConstRoomList = std::vector<std::shared_ptr<const Room>>;
using MarkerList = std::vector<std::shared_ptr<InfoMark>>;

namespace mctp {
struct MapCanvasTexturesProxy;
}

class NODISCARD_QOBJECT MapData final : public MapFrontend
{
    Q_OBJECT

private:
    friend class RoomSelection;

protected:
    MarkerList m_markers;
    // changed data?
    bool m_dataChanged = false;
    bool m_fileReadOnly = false;
    QString m_fileName;
    Coordinate m_position;

protected:
    // the room will be inserted in the given selection. the selection must have been created by mapdata
    NODISCARD const Room *getRoom(const Coordinate &pos, RoomSelection &in);
    NODISCARD const Room *getRoom(RoomId id, RoomSelection &in);

public:
    explicit MapData(QObject *parent);
    ~MapData() override;

    void generateBatches(MapCanvasRoomDrawer &screen, const OptBounds &bounds);

    /* REVISIT: some callers ignore this */
    bool execute(std::unique_ptr<MapAction> action, const SharedRoomSelection &unlock);

    NODISCARD const Coordinate &getPosition() const { return m_position; }
    NODISCARD const MarkerList &getMarkersList() const { return m_markers; }
    NODISCARD uint32_t getRoomsCount() const
    {
        return (greatestUsedId == INVALID_ROOMID) ? 0u : (greatestUsedId.asUint32() + 1u);
    }

    void addMarker(const std::shared_ptr<InfoMark> &im);
    void removeMarker(const std::shared_ptr<InfoMark> &im);
    void removeMarkers(const MarkerList &toRemove);

    NODISCARD bool isEmpty() const
    {
        return (greatestUsedId == INVALID_ROOMID) && m_markers.empty();
    }
    NODISCARD bool dataChanged() const { return m_dataChanged; }
    NODISCARD QList<Coordinate> getPath(const Coordinate &start, const CommandQueue &dirs);

private:
    void virt_clear() final;

public:
    // search for matches
    void genericSearch(RoomRecipient *recipient, const RoomFilter &f);

    void shortestPathSearch(const Room *origin,
                            ShortestPathRecipient *recipient,
                            const RoomFilter &f,
                            int max_hits = -1,
                            double max_dist = 0);

    // Used in Console Commands
    void removeDoorNames();
    NODISCARD const DoorName &getDoorName(const Coordinate &pos, ExitDirEnum dir);

public:
    void setFileName(QString filename, bool readOnly)
    {
        m_fileName = filename;
        m_fileReadOnly = readOnly;
    }
    NODISCARD const QString &getFileName() const { return m_fileName; }
    NODISCARD bool isFileReadOnly() const { return m_fileReadOnly; }

public:
    NODISCARD bool getExitFlag(const Coordinate &pos, ExitDirEnum dir, ExitFieldVariant var);
    NODISCARD ExitDirFlags getExitDirections(const Coordinate &pos);

public:
    NODISCARD const Room *getRoom(const Coordinate &pos);

private:
    // REVISIT: This might be the equivalent of blocking Qt signals.
    bool m_ignoreModifications = false;
    void virt_onNotifyModified(Room &room, const RoomUpdateFlags updateFlags) override
    {
        RoomModificationTracker::virt_onNotifyModified(room, updateFlags);
        if (!m_ignoreModifications) {
            setDataChanged();
        }
    }
    void virt_onNotifyModified(InfoMark &mark, const InfoMarkUpdateFlags updateFlags) override
    {
        InfoMarkModificationTracker::virt_onNotifyModified(mark, updateFlags);
        if (!m_ignoreModifications) {
            setDataChanged();
        }
    }

    void log(const QString &msg) { emit sig_log("MapData", msg); }

public:
    void unsetDataChanged() { m_dataChanged = false; }
    void setDataChanged()
    {
        m_dataChanged = true;
        emit sig_onDataChanged();
    }
    void setPosition(const Coordinate &pos) { m_position = pos; }

signals:
    void sig_log(const QString &, const QString &);
    void sig_onDataChanged();

public slots:
    void slot_scheduleAction(std::shared_ptr<MapAction> action)
    {
        MapFrontend::scheduleAction(action);
    }
};
