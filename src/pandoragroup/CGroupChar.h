#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <vector>
#include <QByteArray>
#include <QColor>
#include <QVariantMap>

#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "../parser/CommandQueue.h"
#include "mmapper2character.h"

class CGroupChar;
using SharedGroupChar = std::shared_ptr<CGroupChar>;

// TODO: make std::vector private
class NODISCARD GroupVector : public std::vector<SharedGroupChar>
{};

class NODISCARD CGroupChar final : public std::enable_shared_from_this<CGroupChar>
{
private:
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };

public:
    RoomId roomId = DEFAULT_ROOMID;
    int hp = 0, maxhp = 0;
    int mana = 0, maxmana = 0;
    int moves = 0, maxmoves = 0;
    CharacterPositionEnum position = CharacterPositionEnum::UNDEFINED;
    CharacterAffects affects;
    CommandQueue prespam;

public:
    CGroupChar() = delete;
    explicit CGroupChar(this_is_private);
    virtual ~CGroupChar();
    DELETE_CTORS_AND_ASSIGN_OPS(CGroupChar);

public:
    static SharedGroupChar alloc() { return std::make_shared<CGroupChar>(this_is_private{0}); }

public:
    NODISCARD const QByteArray &getName() const { return name; }
    void setName(QByteArray _name) { name = _name; }
    void setColor(QColor col) { color = col; }
    NODISCARD const QColor &getColor() const { return color; }
    NODISCARD const QVariantMap toVariantMap() const;
    bool updateFromVariantMap(const QVariantMap &);
    void setRoomId(RoomId id) { roomId = id; }
    NODISCARD RoomId getRoomId() const { return roomId; }
    NODISCARD static QByteArray getNameFromUpdateChar(const QVariantMap &);

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
    QByteArray name;
    QColor color;
};
