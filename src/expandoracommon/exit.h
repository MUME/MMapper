#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef ABSTRACTEXIT_H
#define ABSTRACTEXIT_H

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
    DoorName doorName{};
    ExitFlags exitFlags{};
    DoorFlags doorFlags{};
    bool hasFields = false;

protected:
    RoomIdSet incoming{};
    RoomIdSet outgoing{};

public:
    // This has to exist as long as ExitsList uses QVector<Exit>.
    explicit Exit() = default;
    explicit Exit(bool hasFields)
        : hasFields{hasFields}
    {}

public:
    void clearFields()
    {
        assert(hasFields);
        doorName = DoorName{};
        exitFlags = ExitFlags{};
        doorFlags = DoorFlags{};
    }

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

    DoorName getDoorName() const;
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
};

#endif
