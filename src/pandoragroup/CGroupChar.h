#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
