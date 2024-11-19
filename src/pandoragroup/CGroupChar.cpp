// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"

#include "../global/QuotedQString.h"
#include "../parser/abstractparser.h"

#include <QDebug>
#include <QMessageLogContext>

#define KEY static constexpr const char *const
KEY playerDataKey = "playerData";

KEY nameKey = "name";
KEY labelKey = "label";
KEY externalIdKey = "room"; // external map ids
KEY serverIdKey = "roomid"; // server map ids
KEY colorKey = "color";
KEY stateKey = "state";
KEY prespamKey = "prespam";
KEY affectsKey = "affects";

#undef KEY

CGroupChar::CGroupChar(Badge<CGroupChar>) {}
CGroupChar::~CGroupChar() = default;

void CGroupChar::init(const QString name, const QColor color)
{
    reset();
    setName(name);
    setLabel(name);
    setColor(color);
}

// REVISIT: should this reset the room or not?
void CGroupChar::reset()
{
    // TODO: encapsulate the public members in a struct so they can be reset separately.
    const Internal saved = m_internal;
    *this = CGroupChar{Badge<CGroupChar>{}}; // the actual reset
    m_internal = saved;
}

QVariantMap CGroupChar::toVariantMap() const
{
    QVariantMap playerData;

    playerData[nameKey] = m_internal.name;
    playerData[labelKey] = m_internal.label;
    playerData[colorKey] = m_internal.color.name();
    playerData["hp"] = hp;
    playerData["maxhp"] = maxhp;
    playerData["mana"] = mana;
    playerData["maxmana"] = maxmana;
    playerData["moves"] = moves;
    playerData["maxmoves"] = maxmoves;
    playerData[stateKey] = static_cast<int>(position);
    playerData[externalIdKey] = getExternalId().asUint32();
    playerData[serverIdKey] = getServerId().asUint32();
    playerData[prespamKey] = mmqt::toQByteArray(prespam);
    playerData[affectsKey] = affects.asUint32();

    QVariantMap root;
    root[playerDataKey] = playerData;
    return root;
}

bool CGroupChar::updateFromVariantMap(const QVariantMap &data)
{
    if (!data.contains(playerDataKey) || !data[playerDataKey].canConvert(QMetaType::QVariantMap)) {
        qWarning() << "Unable to find" << QuotedQString(playerDataKey) << "in map" << data;
        return false;
    }
    const QVariantMap &playerData = data[playerDataKey].toMap();

    bool updated = false;
    if (playerData.contains(externalIdKey) && playerData[externalIdKey].canConvert(QMetaType::Int)) {
        const int32_t i = playerData[externalIdKey].toInt();
        const auto newExternalId = [i]() -> ExternalRoomId {
            static constexpr const ExternalRoomId DEFAULT_EXTERNAL_ROOMID{0};
            ExternalRoomId id{static_cast<uint32_t>(i)};
            if (id == INVALID_EXTERNAL_ROOMID) {
                qWarning() << "Invalid external id changed to default external id.";
                return DEFAULT_EXTERNAL_ROOMID;
            }
            return id;
        }();
        if (newExternalId != getExternalId()) {
            updated = true;
            setExternalId(newExternalId);
        }
    }

    if (playerData.contains(serverIdKey) && playerData[serverIdKey].canConvert(QMetaType::Int)) {
        const int32_t i = playerData[serverIdKey].toInt();

        const auto newServerId = [i]() -> ServerRoomId {
            if (i < 1) {
                static constexpr auto ignore = static_cast<int32_t>(
                    INVALID_SERVER_ROOMID.asUint32());
                if (i != ignore) {
                    qWarning() << "Invalid server id.";
                }
                return INVALID_SERVER_ROOMID;
            }
            return ServerRoomId{static_cast<uint32_t>(i)};
        }();
        if (newServerId != getServerId()) {
            updated = true;
            setServerId(newServerId);
        }
    }

    const auto tryUpdateString = [&playerData, &updated](const char *const attr, QString &arr) {
        if (playerData.contains(attr) && playerData[attr].canConvert(QMetaType::QString)) {
            const QString s = playerData[attr].toString();
            if (s != arr) {
                updated = true;
                arr = s;
            }
        }
    };

#define TRY_UPDATE_STRING(s) tryUpdateString(#s, (m_internal.s))
    TRY_UPDATE_STRING(name);
    TRY_UPDATE_STRING(label);
#undef TRY_UPDATE_STRING

    if (playerData.contains(prespamKey) && playerData[prespamKey].canConvert(QMetaType::QString)) {
        // TODO: This should also be a string instead of latin1 byte array.
        const QByteArray ba = mmqt::toQByteArrayLatin1(playerData[prespamKey].toString());
        if (ba != mmqt::toQByteArray(prespam)) {
            updated = true;
            prespam = mmqt::toCommandQueue(ba);
        }
    }

    if (playerData.contains(colorKey) && playerData[colorKey].canConvert(QMetaType::QString)) {
        const QString &str = playerData[colorKey].toString();
        auto &color = m_internal.color;
        if (str != color.name()) {
            updated = true;
            color = QColor(str);
            if (str != color.name()) {
                qWarning() << "Round trip error on color" << QuotedQString(str) << "vs" << color;
            }
        }
    }

    const auto tryUpdateInt = [&playerData, &updated](const char *const attr, int &n) {
        if (playerData.contains(attr) && playerData[attr].canConvert(QMetaType::Int)) {
            const auto i = [&playerData, attr]() {
                const auto tmp = playerData[attr].toInt();
                if (tmp < 0) {
                    qWarning() << "[tryUpdateInt] Input" << attr << "(" << tmp
                               << ") has been raised to 0.";
                    return 0;
                }
                return tmp;
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

#undef UPDATE_AND_BOUNDS_CHECK

    const auto setPosition = [this, &updated](const CharacterPositionEnum newPosition) {
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

    const auto setAffects = [this, &updated](const CharacterAffectFlags newAffects) {
        if (newAffects != affects) {
            updated = true;
            affects = newAffects;
        }
    };

    if (playerData.contains(affectsKey) && playerData[affectsKey].canConvert(QMetaType::UInt)) {
        const uint32_t i = playerData[affectsKey].toUInt();
        setAffects(static_cast<CharacterAffectFlags>(i));
    }

    return updated;
}

QString CGroupChar::getNameFromUpdateChar(const QVariantMap &data)
{
    if (!data.contains(playerDataKey) || !data[playerDataKey].canConvert(QMetaType::QVariantMap)) {
        qWarning() << "Unable to find" << QuotedQString(playerDataKey) << "in map" << data;
        return "";
    }

    const QVariantMap &playerData = data[playerDataKey].toMap();
    if (!playerData.contains(nameKey) || !playerData[nameKey].canConvert(QMetaType::QString)) {
        qWarning() << "Unable to find" << QuotedQString(nameKey) << "in map" << playerData;
        return "";
    }

    return playerData[nameKey].toString();
}
