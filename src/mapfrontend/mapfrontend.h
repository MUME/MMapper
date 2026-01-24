#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/Changes.h"
#include "../map/Map.h"
#include "../map/coordinate.h"
#include "../map/parseevent.h"
#include "../map/roomid.h"
#include "MapHistory.h"

#include <optional>
#include <utility>

#include <QString>
#include <QtCore>

class ParseEvent;
class QObject;
class PathProcessor;

/**
 * The MapFrontend organizes rooms and their relations to each other.
 */
class NODISCARD_QOBJECT MapFrontend : public QObject, public RoomModificationTracker
{
    Q_OBJECT

private:
    struct NODISCARD MapState final
    {
        Map map;

        NODISCARD bool operator==(const MapState &rhs) const { return map == rhs.map; }
        NODISCARD bool operator!=(const MapState &rhs) const { return !(rhs == *this); }
    };

    MapState m_saved;
    MapState m_current;
    MapState m_snapshot;

private:
    void emitUndoRedoAvailability();

    struct HistoryCoordinator
    {
        MapHistory undo_stack;
        MapHistory redo_stack;

        HistoryCoordinator(size_t max_depth)
            : undo_stack(true, max_depth)
            , redo_stack(false)
        {}

        void recordChange(Map prev)
        {
            undo_stack.push(std::move(prev));
            if (!redo_stack.isEmpty()) {
                redo_stack.clear();
            }
        }

        std::optional<Map> undo(Map current_map_state_for_redo)
        {
            if (undo_stack.isEmpty()) {
                return std::nullopt;
            }
            redo_stack.push(std::move(current_map_state_for_redo));
            return undo_stack.pop();
        }

        std::optional<Map> redo(Map current_map_state_for_undo)
        {
            if (redo_stack.isEmpty()) {
                return std::nullopt;
            }
            undo_stack.push(std::move(current_map_state_for_undo));
            return redo_stack.pop();
        }

        bool isUndoAvailable() const { return !undo_stack.isEmpty(); }
        bool isRedoAvailable() const { return !redo_stack.isEmpty(); }

        void clearAll()
        {
            undo_stack.clear();
            redo_stack.clear();
        }
    };
    HistoryCoordinator m_history;

public:
    explicit MapFrontend(QObject *parent);
    ~MapFrontend() override;

public:
    NODISCARD Map getCurrentMap() const { return m_current.map; }
    NODISCARD Map getSavedMap() const { return m_saved.map; }

public:
    NODISCARD bool isModified() const { return m_current != m_saved; }

private:
    void setCurrentMap(const MapApplyResult &);

public:
    void setCurrentMap(Map map);
    void setSavedMap(Map map);
    void currentHasBeenSaved() { m_saved = m_current; }

public:
    void saveSnapshot();
    void restoreSnapshot();

private:
    bool applyChangesInternal(
        ProgressCounter &pc,
        const std::function<MapApplyResult(Map &, ProgressCounter &)> &applyFunction);

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
    NODISCARD bool hasTemporaryRoom(RoomId id) const;
    NODISCARD bool tryRemoveTemporary(RoomId id);
    NODISCARD bool tryMakePermanent(RoomId id);

    NODISCARD RoomHandle findRoomHandle(RoomId) const;
    /// Find first room at coordinate (legacy, prefer findRoomHandles for new code)
    NODISCARD RoomHandle findRoomHandle(const Coordinate &) const;
    NODISCARD RoomHandle findRoomHandle(ExternalRoomId id) const;
    NODISCARD RoomHandle findRoomHandle(ServerRoomId id) const;
    /// Find all rooms at coordinate
    NODISCARD std::vector<RoomHandle> findRoomHandles(const Coordinate &) const;

    NODISCARD RoomHandle getRoomHandle(RoomId id) const;
    NODISCARD const RawRoom &getRawRoom(RoomId id) const;

    /// Find all room IDs at coordinate (can be multiple if rooms overlap)
    NODISCARD RoomIdSet findAllRooms(const Coordinate &) const;
    NODISCARD RoomIdSet findAllRooms(const SigParseEvent &) const;
    // bounding box
    NODISCARD RoomIdSet findAllRooms(const Coordinate &, const Coordinate &) const;

public:
    void scheduleAction(const Change &change);

    // looking for rooms returns a set of matching room IDs
    NODISCARD RoomIdSet lookingForRooms(const SigParseEvent &);

signals:
    // this signal is also sent out if a room is deleted. So any clients still
    // working on this room can start some emergency action.
    void sig_mapSizeChanged(const Coordinate &, const Coordinate &);
    void sig_clearingMap();
    void sig_undoAvailable(bool available);
    void sig_redoAvailable(bool available);

public slots:
    // createRoom creates a room without a lock
    // it will get deleted if no one looks for it for a certain time
    void slot_createRoom(const SigParseEvent &, const Coordinate &);
    void slot_undo();
    void slot_redo();
};
