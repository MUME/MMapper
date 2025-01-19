#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../map/roomid.h"
#include "../parser/CommandQueue.h"
#include "mmapper2character.h"

#include <memory>
#include <vector>

#include <QByteArray>
#include <QColor>
#include <QVariantMap>

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
    RoomId roomId = INVALID_ROOMID;
    int hp = 0;
    int maxhp = 0;
    int mana = 0;
    int maxmana = 0;
    int moves = 0;
    int maxmoves = 0;
    CharacterPositionEnum position = CharacterPositionEnum::UNDEFINED;
    CharacterAffectFlags affects;
    CommandQueue prespam;

private:
    struct NODISCARD Internal final
    {
        QByteArray name;
        QByteArray label;
        QColor color;
    };
    Internal m_internal;

public:
    CGroupChar() = delete;
    explicit CGroupChar(this_is_private);
    virtual ~CGroupChar();
    DELETE_CTORS(CGroupChar);
    DELETE_COPY_ASSIGN_OP(CGroupChar);

private:
    DEFAULT_MOVE_ASSIGN_OP(CGroupChar);

public:
    // uses private move assignment operator
    // TODO: encapsulate the public members in a struct so they can be reset separately.
    void reset()
    {
        const Internal saved = m_internal;
        *this = CGroupChar{this_is_private{0}};
        m_internal = saved;
    }

public:
    static SharedGroupChar alloc() { return std::make_shared<CGroupChar>(this_is_private{0}); }

public:
    NODISCARD const QByteArray &getName() const { return m_internal.name; }
    void setName(QByteArray name) { m_internal.name = name; }
    NODISCARD const QByteArray &getLabel() const { return m_internal.label; }
    void setLabel(QByteArray label) { m_internal.label = label; }
    void setColor(QColor col) { m_internal.color = col; }
    NODISCARD const QColor &getColor() const { return m_internal.color; }
    NODISCARD QVariantMap toVariantMap() const;
    NODISCARD bool updateFromVariantMap(const QVariantMap &);
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
};
