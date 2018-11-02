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

#include "CGroupChar.h"

#include <QCharRef>
#include <QDebug>
#include <QMessageLogContext>

#include "../global/roomid.h"

CGroupChar::CGroupChar() = default;
CGroupChar::~CGroupChar() = default;

const QVariantMap CGroupChar::toVariantMap() const
{
    QVariantMap playerData;

    playerData["name"] = name;
    playerData["color"] = color.name();
    playerData["hp"] = hp;
    playerData["maxhp"] = maxhp;
    playerData["mana"] = mana;
    playerData["maxmana"] = maxmana;
    playerData["moves"] = moves;
    playerData["maxmoves"] = maxmoves;
    playerData["state"] = static_cast<int>(state);
    playerData["room"] = pos.asUint32();

    QVariantMap root;
    root["playerData"] = playerData;
    return root;
}

bool CGroupChar::updateFromVariantMap(const QVariantMap &data)
{
    if (!data.contains("playerData") || !data["playerData"].canConvert(QMetaType::QVariantMap)) {
        qWarning() << "Unable to find 'playerData' in map" << data;
        return false;
    }
    const QVariantMap &playerData = data["playerData"].toMap();

    bool updated = false;
    if (playerData.contains("room") && playerData["room"].canConvert(QMetaType::UInt)) {
        const auto newpos = RoomId{static_cast<uint32_t>(playerData["room"].toUInt())};
        if (newpos != pos) {
            updated = true;
            pos = newpos;
        }
    }

    const auto tryUpdateString = [&playerData, &updated](const char *const attr, QByteArray &arr) {
        if (playerData.contains(attr) && playerData[attr].canConvert(QMetaType::QByteArray)) {
            const auto &s = playerData[attr].toByteArray();
            if (s != arr) {
                updated = true;
                arr = s;
            }
        }
    };

#define TRY_UPDATE_STRING(s) tryUpdateString(#s, (s))

    TRY_UPDATE_STRING(name);

    if (playerData.contains("color") && playerData["color"].canConvert(QMetaType::QString)) {
        const QString &str = playerData["color"].toString();
        if (str != color.name()) {
            updated = true;
            color = QColor(str);
        }
    }

    const auto tryUpdateInt = [&playerData, &updated](const char *const attr, int &n) {
        if (playerData.contains(attr) && playerData[attr].canConvert(QMetaType::Int)) {
            const auto i = playerData[attr].toInt();
            if (i != n) {
                updated = true;
                n = i;
            }
        }
    };
#define TRY_UPDATE_INT(n) tryUpdateInt(#n, (n))

    TRY_UPDATE_INT(hp);
    TRY_UPDATE_INT(maxhp);
    TRY_UPDATE_INT(mana);
    TRY_UPDATE_INT(maxmana);
    TRY_UPDATE_INT(moves);
    TRY_UPDATE_INT(maxmoves);

    if (playerData.contains("state") && playerData["state"].canConvert(QMetaType::Int)) {
        const auto newState = static_cast<CharacterStates>(playerData["state"].toInt());
        if (newState != state) {
            updated = true;
            state = newState;
        }
    }
    return updated;

#undef TRY_UPDATE_INT
#undef TRY_UPDATE_STRING
}

QByteArray CGroupChar::getNameFromVariantMap(const QVariantMap &data)
{
    if (!data.contains("playerData") || !data["playerData"].canConvert(QMetaType::QVariantMap)) {
        qWarning() << "Unable to find 'playerData' in map" << data;
        return "";
    }

    const QVariantMap &playerData = data["playerData"].toMap();
    if (!playerData.contains("name") || !playerData["name"].canConvert(QMetaType::QByteArray)) {
        qWarning() << "Unable to find 'name' in map" << playerData;
        return "";
    }

    return playerData["name"].toByteArray();
}
