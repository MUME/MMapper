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

#ifndef CGROUPCHAR_H_
#define CGROUPCHAR_H_

#include <QByteArray>
#include <QColor>

#include "../global/roomid.h"

class QDomNode;

class CGroupChar
{
public:
    enum class CharacterStates {
        NORMAL,
        FIGHTING,
        RESTING,
        SLEEPING,
        CASTING,
        INCAPACITATED,
        DEAD
        // TODO: Add BLIND BASHED SLEPT POISONED BLEEDING
    };

public:
    RoomId pos = INVALID_ROOMID;
    int hp = 0, maxhp = 0;
    int mana = 0, maxmana = 0;
    int moves = 0, maxmoves = 0;
    CharacterStates state = CharacterStates::NORMAL;

    explicit CGroupChar();
    virtual ~CGroupChar();

    const QByteArray &getName() const { return name; }
    void setName(QByteArray _name) { name = _name; }
    void setColor(QColor col) { color = col; }
    const QColor &getColor() const { return color; }
    const QDomNode toXML() const;
    bool updateFromXML(const QDomNode &node);
    void setPosition(RoomId id) { pos = id; }
    RoomId getPosition() const { return pos; }
    static QByteArray getNameFromXML(const QDomNode &node);

private:
    QByteArray name{};
    QColor color{};

public:
    CGroupChar(const CGroupChar &) = delete;
    CGroupChar &operator=(const CGroupChar &) = delete;
};

class CGroupLocalChar final : public CGroupChar
{
public:
    explicit CGroupLocalChar() = default;

    QByteArray textHP{};
    QByteArray textMoves{};
    QByteArray textMana{};

    // REVISIT: Why does this need to extend CGroupChar?
    void setScore(int _hp, int _maxhp, int _mana, int _maxmana, int _moves, int _maxmoves)
    {
        hp = _hp;
        maxhp = _maxhp;
        mana = _mana;
        maxmana = _maxmana;
        moves = _moves;
        maxmoves = _maxmoves;
    }

    void setTextScore(QByteArray hp, QByteArray mana, QByteArray moves)
    {
        textHP = hp;
        textMana = mana;
        textMoves = moves;
    }
};

#endif /*CGROUPCHAR_H_*/
