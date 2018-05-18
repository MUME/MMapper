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

#include <QDebug>
#include <QDomNode>

CGroupChar::CGroupChar()
{
    name = "";
    pos = 0;
    hp = 0;
    maxhp = 0;
    moves = 0;
    maxmoves = 0;
    mana = 0;
    maxmana = 0;
    state = NORMAL;
    textHP = "";
    textMoves = "";
    textMana = "";
    lastMovement = "";
}

CGroupChar::~CGroupChar()
    = default;

QDomNode CGroupChar::toXML()
{

    QDomDocument doc("charinfo");

    QDomElement root = doc.createElement("playerData");
    root.setAttribute("name", QString(name) );
    root.setAttribute("color", color.name() );
    root.setAttribute("textHP", QString(textHP) );
    root.setAttribute("textMana", QString(textMana) );
    root.setAttribute("textMoves", QString(textMoves) );
    root.setAttribute("hp", hp );
    root.setAttribute("maxhp", maxhp );
    root.setAttribute("mana", mana);
    root.setAttribute("maxmana", maxmana);
    root.setAttribute("moves", moves );
    root.setAttribute("maxmoves", maxmoves );
    root.setAttribute("state", state );
    root.setAttribute("lastMovement", QString(lastMovement));
    root.setAttribute("room", pos );
    doc.appendChild(root);

    return root;
}

bool CGroupChar::updateFromXML(const QDomNode &node)
{
    bool updated;


    updated = false;
    if (node.nodeName() != "playerData") {
        qWarning("Called updateFromXML with wrong node. The name does not fit.");
        return false;
    }

    QString s;
    QByteArray str;
    int newval;

    QDomElement e = node.toElement();

    unsigned int newpos  = e.attribute("room").toInt();
    if (newpos != pos) {
        updated = true;
        pos = newpos;
    }


    str = e.attribute("name").toLatin1();
    if (str != name) {
        updated = true;
        name = str;
    }

    str = e.attribute("lastMovement").toLatin1();
    if (str != lastMovement) {
        updated = true;
        lastMovement = str;
    }


    str = e.attribute("color").toLatin1();
    if (str != color.name().toLatin1()) {
        updated = true;
        color = QColor(QString(str));
    }

    str = e.attribute("textHP").toLatin1();
    if (s != textHP) {
        updated = true;
        textHP = str;
    }

    str = e.attribute("textMana").toLatin1();
    if (s != textMana) {
        updated = true;
        textMana = str;
    }

    str = e.attribute("textMoves").toLatin1();
    if (s != textMoves) {
        updated = true;
        textMoves = str;
    }

    newval  = e.attribute("hp").toInt();
    if (newval != hp) {
        updated = true;
        hp = newval;
    }

    newval  = e.attribute("maxhp").toInt();
    if (newval != maxhp) {
        updated = true;
        maxhp = newval;
    }

    newval  = e.attribute("mana").toInt();
    if (newval != mana) {
        updated = true;
        mana = newval;
    }

    newval  = e.attribute("maxmana").toInt();
    if (newval != maxmana) {
        updated = true;
        maxmana = newval;
    }

    newval  = e.attribute("moves").toInt();
    if (newval != moves) {
        updated = true;
        moves = newval;
    }

    newval  = e.attribute("maxmoves").toInt();
    if (newval != maxmoves) {
        updated = true;
        maxmoves = newval;
    }

    newval  = e.attribute("state").toInt();
    if (newval != state) {
        updated = true;
        state = newval;
    }
    return updated;
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

