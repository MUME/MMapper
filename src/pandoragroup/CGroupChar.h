#pragma once
/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
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

#include <QByteArray>
#include <QColor>
#include <QVariantMap>

#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "../parser/CommandQueue.h"
#include "mmapper2character.h"

class CGroupChar final
{
public:
    RoomId roomId = DEFAULT_ROOMID;
    int hp = 0, maxhp = 0;
    int mana = 0, maxmana = 0;
    int moves = 0, maxmoves = 0;
    CharacterPosition position = CharacterPosition::UNDEFINED;
    CharacterAffects affects{};
    CommandQueue prespam{};

    explicit CGroupChar();
    virtual ~CGroupChar();

    const QByteArray &getName() const { return name; }
    void setName(QByteArray _name) { name = _name; }
    void setColor(QColor col) { color = col; }
    const QColor &getColor() const { return color; }
    const QVariantMap toVariantMap() const;
    bool updateFromVariantMap(const QVariantMap &);
    void setRoomId(RoomId id) { roomId = id; }
    RoomId getRoomId() const { return roomId; }
    static QByteArray getNameFromUpdateChar(const QVariantMap &);

    void setScore(int _hp, int _maxhp, int _mana, int _maxmana, int _moves, int _maxmoves)
    {
        hp = _hp;
        maxhp = _maxhp;
        mana = _mana;
        maxmana = _maxmana;
        moves = _moves;
        maxmoves = _maxmoves;
    }

private:
    QByteArray name{};
    QColor color{};

public:
    DELETE_CTORS_AND_ASSIGN_OPS(CGroupChar);
};
