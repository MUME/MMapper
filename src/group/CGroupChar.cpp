// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"

#include "../global/JsonObj.h"
#include "../global/QuotedQString.h"

#include <QDebug>
#include <QMessageLogContext>

CGroupChar::CGroupChar(Badge<CGroupChar>) {}
CGroupChar::~CGroupChar() = default;

void CGroupChar::setId(const GroupId id)
{
    m_server.id = id;
}

void CGroupChar::reset()
{
    const Internal saved = m_internal;
    *this = CGroupChar{Badge<CGroupChar>{}}; // the actual reset
    m_internal = saved;
    m_server.reset();
}

NODISCARD static CharacterPositionEnum toCharacterPosition(const QString &str)
{
#define X_CASE2(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
        if (str == #lower_case) { \
            return CharacterPositionEnum::UPPER_CASE; \
        } \
    } while (false);
    XFOREACH_CHARACTER_POSITION(X_CASE2)
#undef X_CASE2
    return CharacterPositionEnum::UNDEFINED;
}

NODISCARD static CharacterTypeEnum toCharacterType(const QString &str)
{
#define X_CASE2(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
        if (str == #lower_case) { \
            return CharacterTypeEnum::UPPER_CASE; \
        } \
    } while (false);
    XFOREACH_CHARACTER_TYPE(X_CASE2)
#undef X_CASE2
    return CharacterTypeEnum::UNDEFINED;
}

bool CGroupChar::updateFromGmcp(const JsonObj &obj)
{
    bool updated = false;

    if (auto optInt = obj.getInt("mapid")) {
        const int32_t i = optInt.value();
        const auto newServerId = [i]() -> ServerRoomId {
            static constexpr const ServerRoomId DEFAULT_SERVER_ROOMID{0};
            ServerRoomId id{static_cast<uint32_t>(i)};
            if (id == DEFAULT_SERVER_ROOMID) {
                qWarning() << "Invalid server id changed to default server id.";
                return DEFAULT_SERVER_ROOMID;
            }
            return id;
        }();
        if (newServerId != getServerId()) {
            setServerId(newServerId);
            updated = true;
        }
    }

    const auto tryUpdateString = [&obj, &updated](const char *const attr, QString &arr) {
        if (auto optString = obj.getString(attr)) {
            const auto s = optString.value();
            if (s != arr) {
                arr = s;
                updated = true;
            }
        }
    };

    if (auto optString = obj.getString("name")) {
        const auto name = CharacterName{optString.value()};
        if (name != m_server.name) {
            setName(name);
            updated = true;
        }
    }

    if (auto optString = obj.getString("label")) {
        const auto label = CharacterLabel{optString.value()};
        if (label != m_server.label) {
            setLabel(label);
            updated = true;
        }
    }

    const auto tryUpdateInt = [&obj, &updated](const char *const attr, int &n) {
        if (auto optInt = obj.getInt(attr)) {
            const auto i = [&optInt, attr]() {
                const auto tmp = optInt.value();
                if (tmp < 0) {
                    qWarning() << "[tryUpdateInt] Input" << attr << "(" << tmp
                               << ") has been raised to 0.";
                    return 0;
                }
                return tmp;
            }();

            if (i != n) {
                n = i;
                updated = true;
            }
        }
    };

    const auto boundsCheck =
        [&updated](const char *const xname, auto &x, const char *const maxxname, auto &maxx) {
            if (maxx < 0) {
                qWarning() << "[boundsCheck]" << QuotedQString(maxxname) << "(" << maxx
                           << ") has been raised to 0.";
                maxx = 0;
                updated = true;
            }
            if (x > maxx) {
                qWarning() << "[boundsCheck]" << QuotedQString(xname) << "(" << x
                           << ") has been clamped to " << maxxname << "(" << maxx << ").";
                x = maxx;
                updated = true;
            }
        };

#define UPDATE_AND_BOUNDS_CHECK(n) \
    do { \
        tryUpdateInt(#n, (m_server.n)); \
        tryUpdateInt(("max" #n), (m_server.max##n)); \
        boundsCheck(#n, (m_server.n), ("max" #n), (m_server.max##n)); \
    } while (0)

    UPDATE_AND_BOUNDS_CHECK(hp);
    UPDATE_AND_BOUNDS_CHECK(mana);
    UPDATE_AND_BOUNDS_CHECK(mp);
#undef UPDATE_AND_BOUNDS_CHECK

    if (auto optString = obj.getString("position")) {
        const auto pos = toCharacterPosition(optString.value());
        if (setPosition(pos)) {
            updated = true;
        }
    }

    if (auto optString = obj.getString("type")) {
        const auto newType = toCharacterType(optString.value());
        if (newType != m_server.type) {
            m_server.type = newType;
            updated = true;
            if (newType == CharacterTypeEnum::NPC) {
                m_server.maxhp = 100;
                m_server.maxmp = 100;
            }
        }
    }

    if (auto optString = obj.getString("room")) {
        const auto roomName = CharacterRoomName{optString.value()};
        if (roomName != m_server.roomName) {
            setRoomName(roomName);
            updated = true;
        }
    }

#define X_CASE(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
        if (auto optBool = obj.getBool(#lower_case)) { \
            const bool wasSet = m_server.affects.contains(CharacterAffectEnum::UPPER_CASE); \
            const bool isSet = optBool.value(); \
            if (isSet) { \
                m_server.affects.insert(CharacterAffectEnum::UPPER_CASE); \
            } else { \
                m_server.affects.remove(CharacterAffectEnum::UPPER_CASE); \
            } \
            if (isSet != wasSet) { \
                updated = true; \
            } \
        } \
    } while (false);
    XFOREACH_CHARACTER_AFFECT(X_CASE)
#undef X_CASE

#define TRY_UPDATE_STRING(s, v) tryUpdateString((#s "-string"), (m_server.v))
    TRY_UPDATE_STRING(hp, textHP);
    TRY_UPDATE_STRING(mana, textMana);
    TRY_UPDATE_STRING(mp, textMoves);
#undef TRY_UPDATE_STRING
    if (!obj.getInt("hp").has_value() && !obj.getInt("mp").has_value()
        && !obj.getInt("mana").has_value()
        && setScore(m_server.textHP, m_server.textMana, m_server.textMoves)) {
        updated = true;
    }

    return updated;
}

static bool isDeathHall(const ServerRoomId id)
{
    switch (id.asUint32()) {
    case 1274127:  // Trolls
    case 5495709:  // Orcs
    case 7854852:  // Zaugurz
    case 10578781: // Free People
    case 14623711: // BN
        return true;
    default:
        return false;
    }
}

bool CGroupChar::setPosition(CharacterPositionEnum newPos)
{
    const auto &oldPos = m_server.position;
    if (oldPos == newPos) {
        return false; // No update needed
    }

    // Prefer dead state until we finish recovering some hp (i.e. stand)
    if (newPos != CharacterPositionEnum::STANDING && isDeathHall(getServerId())) {
        if (oldPos == CharacterPositionEnum::DEAD) {
            return false;
        }
        m_server.position = CharacterPositionEnum::DEAD;
    } else {
        m_server.position = newPos;
    }
    return true;
}

bool CGroupChar::setScore(const QString &textHP, const QString &textMana, const QString &textMoves)
{
    bool updated = false;

    const auto &hp = m_server.hp;
    const auto &maxhp = m_server.maxhp;
    const auto &mana = m_server.mana;
    const auto &maxmana = m_server.maxmana;
    const auto &mp = m_server.mp;
    const auto &maxmp = m_server.maxmp;

#define X_SCORE(target, lower, upper) \
    do { \
        if (text == (target)) { \
            if (current >= (upper)) { \
                return upper; \
            } else if (current <= (lower)) { \
                return lower; \
            } else { \
                return current; \
            } \
        } \
    } while (false)

    // Estimate new numerical scores using prompt
    if (maxhp != 0) {
        // REVISIT: Replace this if/else tree with a data structure
        const auto calc_hp =
            [](const QString &text, const double current, const double max) -> double {
            if (text == "healthy") {
                return max;
            }
            X_SCORE("fine", max * 0.71, max * 0.99);
            X_SCORE("hurt", max * 0.46, max * 0.70);
            X_SCORE("wounded", max * 0.26, max * 0.45);
            X_SCORE("bad", max * 0.11, max * 0.25);
            X_SCORE("awful", max * 0.01, max * 0.10);
            return 0.0; // Dying
        };
        int newHp = static_cast<int>(calc_hp(textHP, hp, maxhp));
        if (hp != newHp) {
            m_server.hp = newHp;
            updated = true;
        }
    }
    if (maxmana != 0) {
        const auto calc_mana =
            [](const QString &text, const double current, const double max) -> double {
            if (text == "full") {
                return max;
            }
            X_SCORE("burning", max * 0.76, max * 0.99);
            X_SCORE("hot", max * 0.46, max * 0.75);
            X_SCORE("warm", max * 0.26, max * 0.45);
            X_SCORE("cold", max * 0.11, max * 0.25);
            X_SCORE("icy", max * 0.01, max * 0.10);
            return 0.0; // Frozen
        };
        int newMana = static_cast<int>(calc_mana(textMana, mana, maxmana));
        if (mana != newMana) {
            m_server.mana = newMana;
            updated = true;
        }
    }
    if (maxmp != 0) {
        const auto calc_moves =
            [](const QString &text, const double current, const double max) -> double {
            if (text == "unwearied") {
                return max;
            }
            X_SCORE("steadfast", max * 0.70, max * 0.99);
            X_SCORE("rested", 50, max * 0.69);
            X_SCORE("tired", 30, 49);
            X_SCORE("slow", 15, 29);
            X_SCORE("weak", 5, 14);
            X_SCORE("fainting", 1, 4);
            return 0.0; // Exhausted
        };
        int newMp = static_cast<int>(calc_moves(textMoves, mp, maxmp));
        if (mp != newMp) {
            m_server.mp = newMp;
            updated = true;
        }
    }
#undef X_SCORE
    return updated;
}

QString CGroupChar::getDisplayName() const
{
    if (getLabel().isEmpty()
        || getName().getStdStringViewUtf8() == getLabel().getStdStringViewUtf8()) {
        return getName().toQString();
    } else {
        return QString("%1 (%2)").arg(getName().toQString(), getLabel().toQString());
    }
}
