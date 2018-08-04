#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
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

#ifndef SEARCHTREENODE
#define SEARCHTREENODE

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../global/roomid.h"
#include "ByteArray.h"
#include "tinylist.h"

class AbstractRoomVisitor;
class ParseEvent;

/**
 * keeps a substring of the properties, and a table of other RoomSearchNodes pointing to the possible following characters
 */

class SearchTreeNode
{
protected:
    TinyList children{};
    ByteArray myChars{};

public:
    explicit SearchTreeNode(ParseEvent &event);
    explicit SearchTreeNode(ByteArray in_bytes, TinyList in_children);
    explicit SearchTreeNode() = default;
    virtual ~SearchTreeNode() = default;

public:
    virtual void getRooms(AbstractRoomVisitor &stream, ParseEvent &event);
    virtual SharedRoomCollection insertRoom(ParseEvent &event);
    virtual void setChild(char, SearchTreeNode *);
    virtual void skipDown(AbstractRoomVisitor &stream, ParseEvent &event);
};

#endif
