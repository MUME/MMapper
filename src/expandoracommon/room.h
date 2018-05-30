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

#ifndef ROOM_H
#define ROOM_H

#include "coordinate.h"
#include "exit.h"
#include <mapdata/mmapper2room.h>
#include <QVariant>
#include <QVector>

typedef QVector<Exit> ExitsList;
typedef QVectorIterator<Exit> ExitsListIterator;

class Room final
{
private:
    using RoomFields = QVector<QVariant>;

private:
    uint id = UINT_MAX;
    Coordinate position{};
    bool temporary = true;
    bool upToDate = false;
    RoomFields fields;
    ExitsList exits;

public:
    Exit &exit(uint dir) { return exits[dir]; }
    ExitsList &getExitsList() { return exits; }
    const ExitsList &getExitsList() const { return exits; }
    void setId(uint in) { id = in; }
    void setPosition(const Coordinate &in_c) { position = in_c; }
    uint getId() const { return id; }
    const Coordinate &getPosition() const { return position; }
    bool isTemporary() const
    {
        return temporary; // room is new if no exits are present
    }
    void setPermanent() { temporary = false; }
    bool isUpToDate() const { return upToDate; }
    const Exit &exit(uint dir) const { return exits[dir]; }
    void setUpToDate() { upToDate = true; }
    void setOutDated() { upToDate = false; }

public:
    RoomName getName() const;
    RoomDescription getDescription() const;
    RoomDescription getDynamicDescription() const;
    RoomNote getNote() const;
    RoomMobFlags getMobFlags() const;
    RoomLoadFlags getLoadFlags() const;
    RoomTerrainType getTerrainType() const;
    RoomPortableType getPortableType() const;
    RoomLightType getLightType() const;
    RoomAlignType getAlignType() const;
    RoomRidableType getRidableType() const;
    RoomSundeathType getSundeathType() const;

public:
    inline QVariant at(const RoomField field) const { return fields.at(static_cast<int>(field)); }
    inline void replace(const RoomField field, QVariant value)
    {
        fields.replace(static_cast<int>(field), value);
    }

public:
    template<typename Callback>
    inline void modifyVariant(const RoomField field, Callback &&callback)
    {
        const QVariant oldValue = at(field);
        replace(field, callback(oldValue));
    }

    template<typename Callback>
    inline void modifyUint(const RoomField field, Callback &&callback)
    {
        const uint oldValue = at(field).toUInt();
        replace(field, callback(oldValue));
    }

    template<typename Callback>
    inline void modifyString(const RoomField field, Callback &&callback)
    {
        const QString oldValue = at(field).toString();
        replace(field, callback(oldValue));
    }

public:
    inline auto begin() { return fields.begin(); }
    inline auto end() { return fields.end(); }
    inline auto begin() const { return fields.begin(); }
    inline auto end() const { return fields.end(); }

public:
    Room(uint numProps, uint numExits, uint numExitProps)
        : fields(numProps)
        , exits(numExits, numExitProps)
    {}
    ~Room() = default;
};

#endif
