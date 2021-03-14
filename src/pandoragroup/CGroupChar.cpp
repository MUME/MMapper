// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"

#include <QDebug>
#include <QMessageLogContext>

#include "../global/roomid.h"
#include "../parser/abstractparser.h"

#define KEY static constexpr const char *const
KEY playerDataKey = "playerData";

KEY nameKey = "name";
KEY roomKey = "room";
KEY colorKey = "color";
KEY stateKey = "state";
KEY prespamKey = "prespam";
KEY affectsKey = "affects";

#undef KEY

CGroupChar::CGroupChar(this_is_private){};
CGroupChar::~CGroupChar() = default;

const QVariantMap CGroupChar::toVariantMap() const
{
    QVariantMap playerData;

    playerData[nameKey] = name;
    playerData[colorKey] = color.name();
    playerData["hp"] = hp;
    playerData["maxhp"] = maxhp;
    playerData["mana"] = mana;
    playerData["maxmana"] = maxmana;
    playerData["moves"] = moves;
    playerData["maxmoves"] = maxmoves;
    playerData[stateKey] = static_cast<int>(position);
    playerData[roomKey] = roomId.asUint32();
    playerData[prespamKey] = prespam.toByteArray();
    playerData[affectsKey] = affects.asUint32();

    QVariantMap root;
    root[playerDataKey] = playerData;
    return root;
}

// TODO: move this somewhere more appropriate
struct NODISCARD QuotedQString final
{
private:
    QString str;

public:
    explicit QuotedQString(QString input)
        : str{std::move(input)}
    {}
    friend QDebug &operator<<(QDebug &debug, const QuotedQString &q)
    {
        debug.quote();
        debug << q.str;
        debug.noquote();
        return debug;
    }
};

bool CGroupChar::updateFromVariantMap(const QVariantMap &data)
{
    if (!data.contains(playerDataKey) || !data[playerDataKey].canConvert(QMetaType::QVariantMap)) {
        qWarning() << "Unable to find" << QuotedQString(playerDataKey) << "in map" << data;
        return false;
    }
    const QVariantMap &playerData = data[playerDataKey].toMap();

    bool updated = false;
    if (playerData.contains(roomKey) && playerData[roomKey].canConvert(QMetaType::UInt)) {
        const uint32_t i = playerData[roomKey].toUInt();

        // FIXME: Using a room# assumes everyone has _exactly_ the same static map;
        // if anyone in the group modifies their map, they will report a room #
        // that does not exist in anyone else's map. And if two different users
        // each modify their maps, they'll be reported as being in the same room?
        // Instead, this should serialize the room coordinates + name/desc/exits.

        // NOTE: We don't have access to the map here, so we can't verify the room #.
        const auto newRoomId = [i]() {
            auto newRoomId = RoomId{i};
            if (newRoomId == INVALID_ROOMID) {
                qWarning() << "Invalid room changed to default room.";
                newRoomId = DEFAULT_ROOMID;
            }
            return newRoomId;
        }();

        if (newRoomId != roomId) {
            updated = true;
            roomId = newRoomId;
        }
    }

    const auto tryUpdateString = [&playerData, &updated](const char *const attr, QByteArray &arr) {
        if (playerData.contains(attr) && playerData[attr].canConvert(QMetaType::QString)) {
            const auto &s = playerData[attr].toString().toLatin1();
            if (s != arr) {
                updated = true;
                arr = s;
            }
        }
    };

#define TRY_UPDATE_STRING(s) tryUpdateString(#s, (s))

    TRY_UPDATE_STRING(name);

    if (playerData.contains(prespamKey) && playerData[prespamKey].canConvert(QMetaType::QString)) {
        const QByteArray &ba = playerData[prespamKey].toString().toLatin1();
        if (ba != prespam.toByteArray()) {
            updated = true;
            prespam = ba;
        }
    }

    if (playerData.contains(colorKey) && playerData[colorKey].canConvert(QMetaType::QString)) {
        const QString &str = playerData[colorKey].toString();
        if (str != color.name()) {
            updated = true;
            color = QColor(str);
            if (str != color.name())
                qWarning() << "Round trip error on color" << QuotedQString(str) << "vs" << color;
        }
    }

    const auto tryUpdateInt = [&playerData, &updated](const char *const attr, int &n) {
        if (playerData.contains(attr) && playerData[attr].canConvert(QMetaType::Int)) {
            const auto i = [&playerData, attr]() {
                auto i = playerData[attr].toInt();
                if (i < 0) {
                    qWarning() << "[tryUpdateInt] Input" << attr << "(" << i
                               << ") has been raised to 0.";
                    i = 0;
                }
                return i;
            }();

            if (i != n) {
                updated = true;
                n = i;
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
        tryUpdateInt(#n, (n)); \
        tryUpdateInt(("max" #n), (max##n)); \
        boundsCheck(#n, (n), ("max" #n), (max##n)); \
    } while (0)

    UPDATE_AND_BOUNDS_CHECK(hp);
    UPDATE_AND_BOUNDS_CHECK(mana);
    UPDATE_AND_BOUNDS_CHECK(moves);

    const auto setPosition = [&position = this->position,
                              &updated](const CharacterPositionEnum newPosition) {
        if (newPosition != position) {
            updated = true;
            position = newPosition;
        }
    };

    if (playerData.contains(stateKey) && playerData[stateKey].canConvert(QMetaType::Int)) {
        const int n = playerData[stateKey].toInt();
        if (n < static_cast<int>(CharacterPositionEnum::UNDEFINED)
            || n >= static_cast<int>(NUM_CHARACTER_POSITIONS)) {
            qWarning() << "Invalid input state (" << n << ") is changed to UNDEFINED.";
            setPosition(CharacterPositionEnum::UNDEFINED);
        } else {
            setPosition(static_cast<CharacterPositionEnum>(n));
        }
    }

    const auto setAffects = [&affects = this->affects, &updated](const CharacterAffects newAffects) {
        if (newAffects != affects) {
            updated = true;
            affects = newAffects;
        }
    };

    if (playerData.contains(affectsKey) && playerData[affectsKey].canConvert(QMetaType::UInt)) {
        const uint32_t i = playerData[affectsKey].toUInt();
        setAffects(static_cast<CharacterAffects>(i));
    }

    return updated;

#undef UPDATE_AND_BOUNDS_CHECK
#undef TRY_UPDATE_STRING
}

QByteArray CGroupChar::getNameFromUpdateChar(const QVariantMap &data)
{
    if (!data.contains(playerDataKey) || !data[playerDataKey].canConvert(QMetaType::QVariantMap)) {
        qWarning() << "Unable to find" << QuotedQString(playerDataKey) << "in map" << data;
        return "";
    }

    const QVariantMap &playerData = data[playerDataKey].toMap();
    if (!playerData.contains(nameKey) || !playerData[nameKey].canConvert(QMetaType::QByteArray)) {
        qWarning() << "Unable to find" << QuotedQString(nameKey) << "in map" << playerData;
        return "";
    }

    return playerData[nameKey].toByteArray();
}
