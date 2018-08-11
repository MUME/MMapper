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
#include <QDomNode>
#include <QMessageLogContext>

#include "../global/roomid.h"

CGroupChar::CGroupChar() = default;
CGroupChar::~CGroupChar() = default;

const QDomNode CGroupChar::toXML() const
{
    QDomDocument doc("charinfo");

    QDomElement root = doc.createElement("playerData");
    root.setAttribute("name", QString(name));
    root.setAttribute("color", color.name());
    root.setAttribute("hp", hp);
    root.setAttribute("maxhp", maxhp);
    root.setAttribute("mana", mana);
    root.setAttribute("maxmana", maxmana);
    root.setAttribute("moves", moves);
    root.setAttribute("maxmoves", maxmoves);
    root.setAttribute("state", static_cast<int>(state));
    root.setAttribute("room", pos.asUint32());
    doc.appendChild(root);

    return root;
}

bool CGroupChar::updateFromXML(const QDomNode &node)
{
    if (node.nodeName() != "playerData") {
        qWarning("Called updateFromXML with wrong node. The name does not fit.");
        return false;
    }

    bool updated = false;
    QDomElement e = node.toElement();

    {
        auto newpos = RoomId{e.attribute("room").toUInt()};
        if (newpos != pos) {
            updated = true;
            pos = newpos;
        }
    }

    auto tryUpdateString = [&](const char *attr, QByteArray &arr) {
        auto s = e.attribute(attr).toLatin1();
        if (s == arr)
            return;

        arr = s;
        updated = true;
    };

#define TRY_UPDATE_STRING(s) tryUpdateString(#s, (s))

    TRY_UPDATE_STRING(name);

    auto str = e.attribute("color");
    if (str != color.name()) {
        updated = true;
        color = QColor(str);
    }

    auto tryUpdateInt = [&](const char *attr, int &n) {
        auto i = e.attribute(attr).toInt();
        if (i == n)
            return;

        n = i;
        updated = true;
    };
#define TRY_UPDATE_INT(n) tryUpdateInt(#n, (n))

    TRY_UPDATE_INT(hp);
    TRY_UPDATE_INT(maxhp);
    TRY_UPDATE_INT(mana);
    TRY_UPDATE_INT(maxmana);
    TRY_UPDATE_INT(moves);
    TRY_UPDATE_INT(maxmoves);

    {
        auto newState = static_cast<CharacterStates>(e.attribute("state").toInt());
        if (newState != state) {
            updated = true;
            state = newState;
        }
    }
    return updated;

#undef TRY_UPDATE_INT
#undef TRY_UPDATE_STRING
}

QByteArray CGroupChar::getNameFromXML(const QDomNode &node)
{
    if (node.nodeName() != "playerData") {
        qWarning("Called updateFromXML with wrong node. The name does not fit.");
        return QByteArray();
    }

    QDomElement e = node.toElement();

    return e.attribute("name").toLatin1();
}
