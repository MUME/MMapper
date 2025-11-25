#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Badge.h"
#include "../global/RuleOf5.h"
#include "../global/hash.h"
#include "../map/roomid.h"
#include "mmapper2character.h"

#include <memory>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QColor>

class CGroupChar;
using SharedGroupChar = std::shared_ptr<CGroupChar>;
class JsonObj;

namespace tags {
struct NODISCARD GroupIdTag final
{};
} // namespace tags

struct NODISCARD GroupId final : public TaggedInt<GroupId, tags::GroupIdTag, uint32_t>
{
    using TaggedInt::TaggedInt;
    constexpr GroupId()
        : GroupId{UINT_MAX}
    {}
    NODISCARD constexpr uint32_t asUint32() const { return value(); }
    friend std::ostream &operator<<(std::ostream &os, GroupId id);
    friend AnsiOstream &operator<<(AnsiOstream &os, GroupId id);
};

static_assert(sizeof(GroupId) == sizeof(uint32_t));
static constexpr const GroupId INVALID_GROUPID{UINT_MAX};
static_assert(GroupId{} == INVALID_GROUPID);

template<>
struct std::hash<GroupId>
{
    std::size_t operator()(const GroupId id) const noexcept { return numeric_hash(id.asUint32()); }
};

// TODO: make std::vector private
using GroupVector = std::vector<SharedGroupChar>;

class NODISCARD CGroupChar final : public std::enable_shared_from_this<CGroupChar>
{
private:
    struct NODISCARD Internal final
    {
        QColor color = QColor::Invalid;
    };

private:
    Internal m_internal;

private:
    QString m_characterToken;

private:
    struct NODISCARD Server final
    {
        GroupId id = INVALID_GROUPID;
        CharacterName name;
        CharacterLabel label;
        ServerRoomId serverId = INVALID_SERVER_ROOMID;

        CharacterRoomName roomName;
        QString textHP;
        QString textMoves;
        QString textMana;
        int hp = 0;
        int maxhp = 0;
        int mana = 0;
        int maxmana = 0;
        int mp = 0;
        int maxmp = 0;

        CharacterPositionEnum position = CharacterPositionEnum::UNDEFINED;
        CharacterTypeEnum type = CharacterTypeEnum::UNDEFINED;
        CharacterAffectFlags affects;

        void reset() { *this = Server{}; }
    };

private:
    Server m_server;

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
    void reset();

public:
#define X_DECL_GETTERS_AND_SETTERS(UPPER_CASE, lower_case, CamelCase, friendly) \
    NODISCARD inline bool is##CamelCase() const \
    { \
        return m_server.type == CharacterTypeEnum::UPPER_CASE; \
    }
    XFOREACH_CHARACTER_TYPE(X_DECL_GETTERS_AND_SETTERS)
#undef X_DECL_GETTERS_AND_SETTERS

    /* ---------- temporary helper until GMCP flags real mounts ---------- */
    inline bool isMount() const
    {
        return isNpc(); // treat every NPC as a “mount” for now
    }

public:
    NODISCARD GroupId getId() const
    {
        return m_server.id;
    }
    void setId(GroupId id);
    NODISCARD const CharacterName &getName() const
    {
        return m_server.name;
    }
    void setName(CharacterName name)
    {
        m_server.name = std::move(name);
    }
    NODISCARD const CharacterLabel &getLabel() const
    {
        return m_server.label;
    }
    void setLabel(CharacterLabel label)
    {
        m_server.label = std::move(label);
    }

    QString getDisplayName() const;

    void setColor(QColor col)
    {
        m_internal.color = std::move(col);
    }
    void setServerId(ServerRoomId id)
    {
        m_server.serverId = id;
    }
    NODISCARD const QColor &getColor() const
    {
        return m_internal.color;
    }
    void setCharacterToken(const QString &tokenPath) { m_characterToken = tokenPath; }

    NODISCARD const QString &getCharacterToken() const { return m_characterToken; }
    NODISCARD bool updateFromGmcp(const JsonObj &obj);
    NODISCARD ServerRoomId getServerId() const
    {
        return m_server.serverId;
    }
    void setType(CharacterTypeEnum type)
    {
        m_server.type = type;
    }
    NODISCARD CharacterTypeEnum getType() const
    {
        return m_server.type;
    }
    NODISCARD CharacterPositionEnum getPosition() const
    {
        return m_server.position;
    }
    NODISCARD const CharacterAffectFlags &getAffects() const
    {
        return m_server.affects;
    }
    NODISCARD int getHits() const
    {
        return m_server.hp;
    }
    NODISCARD int getMaxHits() const
    {
        return m_server.maxhp;
    }
    NODISCARD int getMana() const
    {
        return m_server.mana;
    }
    NODISCARD int getMaxMana() const
    {
        return m_server.maxmana;
    }
    NODISCARD int getMoves() const
    {
        return m_server.mp;
    }
    NODISCARD int getMaxMoves() const
    {
        return m_server.maxmp;
    }
    bool setPosition(CharacterPositionEnum pos);
    bool setScore(const QString &hp, const QString &mana, const QString &moves);
    void setRoomName(CharacterRoomName name)
    {
        m_server.roomName = std::move(name);
    }
    NODISCARD const CharacterRoomName &getRoomName() const
    {
        return m_server.roomName;
    }

    void setScore(int _hp, int _maxhp, int _mana, int _maxmana, int _moves, int _maxmoves)
    {
        m_server.hp = _hp;
        m_server.maxhp = _maxhp;
        m_server.mana = _mana;
        m_server.maxmana = _maxmana;
        m_server.mp = _moves;
        m_server.maxmp = _maxmoves;
    }
};
