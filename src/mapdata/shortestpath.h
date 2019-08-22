#pragma once
/************************************************************************
**
** Authors:   ethorondil
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

#include <QSet>
#include <QVector>

#include "../expandoracommon/RoomAdmin.h"
#include "../parser/abstractparser.h"
#include "ExitDirection.h"
#include "mmapper2exit.h"

class Room;
class RoomAdmin;

class SPNode
{
public:
    const Room *r = nullptr;
    int parent = 0;
    double dist = 0.0;
    ExitDirection lastdir{};
    explicit SPNode() = default;
    explicit SPNode(const Room *r, const int parent, double dist, ExitDirection lastdir)
        : r(r)
        , parent(parent)
        , dist(dist)
        , lastdir(lastdir)
    {}
};

class ShortestPathRecipient
{
public:
    virtual void receiveShortestPath(RoomAdmin *admin, QVector<SPNode> spnodes, int endpoint) = 0;
    virtual ~ShortestPathRecipient();
};
