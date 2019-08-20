#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <cassert>
#include <set>
#include <stdexcept>
#include <QVariant>
#include <QVector>

#include "../global/range.h"
#include "../global/roomid.h"
#include "../mapdata/DoorFlags.h"
#include "../mapdata/ExitFieldVariant.h"
#include "../mapdata/ExitFlags.h"
#include "../mapdata/mmapper2exit.h"

class Exit final
{
private:
    DoorName doorName;
    ExitFlags exitFlags;
    DoorFlags doorFlags;

private:
    RoomIdSet incoming;
    RoomIdSet outgoing;

public:
    // This has to exist as long as ExitsList uses QVector<Exit>.
    Exit() = default;
    ~Exit() = default;

public:
    void clearFields()
    {
        doorName = DoorName{};
        exitFlags = ExitFlags{};
        doorFlags = DoorFlags{};
    }

public:
    const RoomIdSet &getIncoming() const { return incoming; }
    const RoomIdSet &getOutgoing() const { return outgoing; }

public:
    auto inSize() const { return incoming.size(); }
    bool inIsEmpty() const { return inSize() == 0; }
    auto inRange() const { return make_range(inBegin(), inEnd()); }
    RoomIdSet inClone() const { return incoming; }

public:
    auto outSize() const { return outgoing.size(); }
    bool outIsEmpty() const { return outSize() == 0; }
    bool outIsUnique() const { return outSize() == 1; }
    RoomId outFirst() const
    {
        assert(!outIsEmpty());
        return *outgoing.begin();
    }
    auto outRange() const { return make_range(outBegin(), outEnd()); }
    RoomIdSet outClone() const { return outgoing; }

public:
    auto getRange(bool out) const { return out ? outRange() : inRange(); }

private:
    std::set<RoomId>::const_iterator inBegin() const { return incoming.begin(); }
    std::set<RoomId>::const_iterator outBegin() const { return outgoing.begin(); }

    std::set<RoomId>::const_iterator inEnd() const { return incoming.end(); }
    std::set<RoomId>::const_iterator outEnd() const { return outgoing.end(); }

public:
    void addIn(RoomId from) { incoming.insert(from); }
    void addOut(RoomId to) { outgoing.insert(to); }
    void removeIn(RoomId from) { incoming.erase(from); }
    void removeOut(RoomId to) { outgoing.erase(to); }
    bool containsIn(RoomId from) const { return incoming.find(from) != incoming.end(); }
    bool containsOut(RoomId to) const { return outgoing.find(to) != outgoing.end(); }

    const DoorName &getDoorName() const;
    bool hasDoorName() const;
    ExitFlags getExitFlags() const;
    DoorFlags getDoorFlags() const;

    void setDoorName(DoorName doorName);
    void setExitFlags(ExitFlags flags);
    void setDoorFlags(DoorFlags flags);

    void clearDoorName();
    void updateExit(ExitFlags flags);

public:
    /* older aliases */
    inline bool isDoor() const { return exitIsDoor(); }
    inline bool isExit() const { return exitIsExit(); }
    inline bool isHiddenExit() const { return doorIsHidden(); }

public:
#define X_DECLARE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool exitIs##CamelCase() const;
    X_FOREACH_EXIT_FLAG(X_DECLARE_ACCESSORS)
#undef X_DECLARE_ACCESSORS

public:
#define X_DECLARE_ACCESSORS(UPPER_CASE, lower_case, CamelCase, friendly) \
    bool doorIs##CamelCase() const;
    X_FOREACH_DOOR_FLAG(X_DECLARE_ACCESSORS)
#undef X_DECLARE_ACCESSORS

    bool doorNeedsKey() const; // REVISIT: This name is not like the others

public:
    bool operator==(const Exit &rhs) const;
    bool operator!=(const Exit &rhs) const;

public:
    Exit(Exit &&) = default;
    Exit(const Exit &) = default;
    Exit &operator=(Exit &&) = default;
    Exit &operator=(const Exit &rhs)
    {
        assignFrom(rhs);
        return *this;
    }

private:
    void assignFrom(const Exit &rhs);
};
