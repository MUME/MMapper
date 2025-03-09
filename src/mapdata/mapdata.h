#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../display/IMapBatchesFinisher.h"
#include "../map/Changes.h"
#include "../map/DoorFlags.h"
#include "../map/ExitDirection.h"
#include "../map/ExitFieldVariant.h"
#include "../map/ExitFlags.h"
#include "../map/Map.h"
#include "../map/coordinate.h"
#include "../map/mmapper2room.h"
#include "../map/roomid.h"
#include "../mapfrontend/mapfrontend.h"
#include "../parser/CommandQueue.h"
#include "MarkerList.h"
#include "roomfilter.h"
#include "roomselection.h"
#include "shortestpath.h"

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include <QList>
#include <QString>
#include <QtCore>
#include <QtGlobal>

class ExitFieldVariant;
class ProgressCounter;
class QObject;
class RoomFieldVariant;
class RoomFilter;
class ShortestPathRecipient;

struct MapCanvasTextures;
struct RawMapData;
struct MapLoadData;
struct RawMapLoadData;

namespace mctp {
struct MapCanvasTexturesProxy;
}

using ConstRoomList = std::vector<RoomHandle>;

class NODISCARD_QOBJECT MapData final : public MapFrontend
{
    Q_OBJECT

private:
    friend class RoomSelection;

private:
    bool m_fileReadOnly = false;
    QString m_fileName;
    std::optional<RoomId> m_selectedRoom;

public:
    explicit MapData(QObject *parent);
    ~MapData() final;

    NODISCARD FutureSharedMapBatchFinisher
    generateBatches(const mctp::MapCanvasTexturesProxy &textures);

    // REVISIT: convert to template, or functionref after it compiles everywhere?
    void applyChangesToList(const RoomSelection &sel,
                            const std::function<Change(const RawRoom &)> &callback);

    NODISCARD std::optional<RoomId> getCurrentRoomId() const { return m_selectedRoom; }

    NODISCARD RoomHandle getCurrentRoom() const
    {
        if (m_selectedRoom) {
            return findRoomHandle(*m_selectedRoom);
        }
        return RoomHandle{};
    }

    NODISCARD std::optional<Coordinate> tryGetPosition() const
    {
        if (const auto &opt_room = getCurrentRoom()) {
            return opt_room.getPosition();
        }
        return std::nullopt;
    }

    NODISCARD InfomarkDb getMarkersList() const { return MapFrontend::getCurrentMarks(); }
    NODISCARD bool isEmpty() const;

    NODISCARD InfomarkId addMarker(const InfoMarkFields &im);
    NODISCARD bool updateMarker(InfomarkId id, const InfoMarkFields &im);
    NODISCARD bool updateMarkers(const std::vector<InformarkChange> &updates);
    void removeMarkers(const MarkerList &toRemove);
    NODISCARD bool removeMarker(InfomarkId id);

    NODISCARD bool dataChanged() const { return MapFrontend::isModified(); }
    void describeChanges(std::ostream &os) const;
    NODISCARD std::string describeChanges() const;

    NODISCARD std::vector<Coordinate> getPath(RoomId start, const CommandQueue &dirs) const;
    NODISCARD std::optional<RoomId> getLast(RoomId start, const CommandQueue &dirs) const;

private:
    void virt_clear() final;

public:
    // search for matches
    NODISCARD RoomIdSet genericFind(const RoomFilter &f) const;

    static void shortestPathSearch(const RoomHandle &origin,
                                   ShortestPathRecipient &recipient,
                                   const RoomFilter &f,
                                   int max_hits = -1,
                                   double max_dist = 0);

    // Used in Console Commands
    void removeDoorNames(ProgressCounter &pc);
    void generateBaseMap(ProgressCounter &pc);
    NODISCARD DoorName getDoorName(RoomId id, ExitDirEnum dir);

public:
    void setMapData(const MapLoadData &mapLoadData);
    NODISCARD static std::pair<Map, InfomarkDb> mergeMapData(ProgressCounter &,
                                                             const Map &currentMap,
                                                             const InfomarkDb &currentMarks,
                                                             RawMapLoadData newMapData);

public:
    void setFileName(QString filename, const bool readOnly)
    {
        m_fileName = std::move(filename);
        m_fileReadOnly = readOnly;
    }
    NODISCARD const QString &getFileName() const { return m_fileName; }
    NODISCARD bool isFileReadOnly() const { return m_fileReadOnly; }

public:
    NODISCARD ExitDirFlags getExitDirections(const Coordinate &pos);

private:
    void virt_onNotifyModified(const RoomUpdateFlags /*updateFlags*/) final { setDataChanged(); }
    void virt_onNotifyModified(const InfoMarkUpdateFlags /*updateFlags*/) final
    {
        setDataChanged();
    }

    void log(const QString &msg) { emit sig_log("MapData", msg); }
    void setDataChanged() { emit sig_onDataChanged(); }

public:
    void clearSelectedRoom() { m_selectedRoom.reset(); }

    void setRoom(const RoomId id)
    {
        auto before = m_selectedRoom;
        if (const auto &room = findRoomHandle(id)) {
            m_selectedRoom = id;
        } else {
            clearSelectedRoom();
        }
        if (before != m_selectedRoom) {
            emit sig_onPositionChange();
        }
    }

    void setPosition(const Coordinate &pos)
    {
        if (const auto &room = findRoomHandle(pos)) {
            setRoom(room.getId());
        } else {
            auto before = m_selectedRoom;
            clearSelectedRoom();
            if (before != m_selectedRoom) {
                emit sig_onPositionChange();
            }
        }
    }

public:
    void forceToRoom(RoomId id)
    {
        auto before = m_selectedRoom;
        setRoom(id);
        if (before != m_selectedRoom) {
            emit sig_onForcedPositionChange();
        }
    }

    void forcePosition(const Coordinate &pos)
    {
        if (const auto &room = findRoomHandle(pos)) {
            forceToRoom(room.getId());
        } else {
            auto before = m_selectedRoom;
            clearSelectedRoom();
            if (before != m_selectedRoom) {
                emit sig_onForcedPositionChange();
            }
        }
    }

private:
    void removeMissing(RoomIdSet &set) const;

signals:
    void sig_log(const QString &, const QString &);
    void sig_onDataChanged();
    void sig_onPositionChange();
    void sig_onForcedPositionChange();
    void sig_checkMapConsistency();
    void sig_generateBaseMap();

public slots:
    void slot_scheduleAction(const SigMapChangeList &);
};
