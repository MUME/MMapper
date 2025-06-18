#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/Changes.h"
#include "../map/Map.h"
#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../map/parseevent.h"
#include "../map/roomid.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <utility>

#include <QString>
#include <QtCore>

class ParseEvent;
class QObject;
class RoomRecipient;

/**
 * The MapFrontend organizes rooms and their relations to each other.
 */
class NODISCARD_QOBJECT MapFrontend : public QObject,
                                      public RoomModificationTracker,
                                      public InfoMarkModificationTracker
{
    Q_OBJECT

private:
    struct NODISCARD MapState final
    {
        InfomarkDb marks;
        Map map;

        NODISCARD bool operator==(const MapState &rhs) const
        {
            return marks == rhs.marks && map == rhs.map;
        }
        NODISCARD bool operator!=(const MapState &rhs) const { return !(rhs == *this); }
    };

    MapState m_saved;
    MapState m_current;
    MapState m_snapshot;

public:
    explicit MapFrontend(QObject *parent);
    ~MapFrontend() override;

public:
    NODISCARD InfomarkDb getCurrentMarks() const { return m_current.marks; }
    NODISCARD InfomarkDb getSavedMarks() const { return m_saved.marks; }
    NODISCARD Map getCurrentMap() const { return m_current.map; }
    NODISCARD Map getSavedMap() const { return m_saved.map; }

public:
    NODISCARD bool isModified() const { return m_current != m_saved; }

private:
    void setCurrentMap(const MapApplyResult &);

public:
    void setCurrentMap(Map map);
    void setCurrentMarks(InfomarkDb marks, InfoMarkUpdateFlags modified);
    void setSavedMarks(InfomarkDb marks);
    void setSavedMap(Map map);
    void currentHasBeenSaved() { m_saved = m_current; }

public:
    void saveSnapshot();
    void restoreSnapshot();

public:
    void setCurrentMarks(InfomarkDb marks)
    {
        setCurrentMarks(std::move(marks), ~InfoMarkUpdateFlags{});
    }
    NODISCARD InfomarkDb getInfomarkDb() const { return getCurrentMarks(); }

public:
    ALLOW_DISCARD bool applyChanges(const ChangeList &changes);
    ALLOW_DISCARD bool applyChanges(ProgressCounter &pc, const ChangeList &changes);
    ALLOW_DISCARD bool applySingleChange(const Change &);
    ALLOW_DISCARD bool applySingleChange(ProgressCounter &pc, const Change &);

private:
    virtual void virt_clear() = 0;

public:
    void revert();
    void clear();
    void block();
    void unblock();
    void checkSize();

    NODISCARD bool createEmptyRoom(const Coordinate &);

    NODISCARD RoomHandle findRoomHandle(RoomId) const;
    NODISCARD RoomHandle findRoomHandle(const Coordinate &) const;
    NODISCARD RoomHandle findRoomHandle(ExternalRoomId id) const;
    NODISCARD RoomHandle findRoomHandle(ServerRoomId id) const;

    NODISCARD RoomHandle getRoomHandle(RoomId id) const;
    NODISCARD const RawRoom &getRawRoom(RoomId id) const;

    // Will technically be 0 or 1
    NODISCARD RoomIdSet findAllRooms(const Coordinate &) const;
    NODISCARD RoomIdSet findAllRooms(const SigParseEvent &) const;
    // bounding box
    NODISCARD RoomIdSet findAllRooms(const Coordinate &, const Coordinate &) const;

public:
    void scheduleAction(const Change &change);

    // looking for rooms leads to a bunch of foundRoom() signals
    void lookingForRooms(RoomRecipient &, const SigParseEvent &);
    void lookingForRooms(RoomRecipient &, RoomId); // by id
    void lookingForRooms(RoomRecipient &, const Coordinate &);

public:
    void keepRoom(RoomRecipient &, RoomId);
    void releaseRoom(RoomRecipient &, RoomId);

signals:
    // this signal is also sent out if a room is deleted. So any clients still
    // working on this room can start some emergency action.
    void sig_mapSizeChanged(const Coordinate &, const Coordinate &);
    void sig_clearingMap();

public slots:
    // createRoom creates a room without a lock
    // it will get deleted if no one looks for it for a certain time
    void slot_createRoom(const SigParseEvent &, const Coordinate &);
};
