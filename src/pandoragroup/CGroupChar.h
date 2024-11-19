#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Badge.h"
#include "../global/RuleOf5.h"
#include "../map/roomid.h"
#include "../parser/CommandQueue.h"
#include "mmapper2character.h"

#include <memory>
#include <utility>
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
    struct NODISCARD Internal final
    {
        QString name;
        QString label;
        QColor color;
        ServerRoomId serverId = INVALID_SERVER_ROOMID;
        ExternalRoomId externalId = INVALID_EXTERNAL_ROOMID;
    };

private:
    Internal m_internal;

public:
    int hp = 0;
    int maxhp = 0;
    int mana = 0;
    int maxmana = 0;
    int moves = 0;
    int maxmoves = 0;
    CharacterPositionEnum position = CharacterPositionEnum::UNDEFINED;
    CharacterAffectFlags affects;
    CommandQueue prespam;

public:
    CGroupChar() = delete;
    explicit CGroupChar(Badge<CGroupChar>);
    virtual ~CGroupChar();
    DELETE_CTORS(CGroupChar);
    DELETE_COPY_ASSIGN_OP(CGroupChar);

private:
    DEFAULT_MOVE_ASSIGN_OP(CGroupChar);

public:
    NODISCARD static SharedGroupChar alloc()
    {
        return std::make_shared<CGroupChar>(Badge<CGroupChar>{});
    }

public:
    void init(QByteArray name, QColor color) = delete;
    void init(QString name, QColor color);
    void reset();

public:
    NODISCARD const QString &getName() const { return m_internal.name; }
    void setName(QByteArray name) = delete;
    void setName(QString name) { m_internal.name = std::move(name); }
    NODISCARD const QString &getLabel() const { return m_internal.label; }
    void setLabel(QByteArray name) = delete;
    void setLabel(QString label) { m_internal.label = std::move(label); }
    void setColor(QColor col) { m_internal.color = std::move(col); }
    void setExternalId(ExternalRoomId id) { m_internal.externalId = id; }
    void setServerId(ServerRoomId id) { m_internal.serverId = id; }
    NODISCARD const QColor &getColor() const { return m_internal.color; }
    NODISCARD QVariantMap toVariantMap() const;
    NODISCARD bool updateFromVariantMap(const QVariantMap &);
    NODISCARD ExternalRoomId getExternalId() const { return m_internal.externalId; }
    NODISCARD ServerRoomId getServerId() const { return m_internal.serverId; }
    NODISCARD static QString getNameFromUpdateChar(const QVariantMap &);

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
