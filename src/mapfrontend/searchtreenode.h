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

#include <memory>
#include <vector>
#include "tinylist.h"

class ParseEvent;

class RoomCollection;

class RoomOutStream;

/**
 * keeps a substring of the properties, and a table of other RoomSearchNodes pointing to the possible following characters
 */

class SearchTreeNode
{
protected:
    using byte_array = std::vector<uint8_t>;
    static byte_array from_string(const char *s);
    static byte_array skip(const byte_array &input, size_t count);
protected:
    TinyList children{};
    byte_array myChars{};
public:
    explicit SearchTreeNode(ParseEvent &event);

    explicit SearchTreeNode(byte_array in_bytes, TinyList in_children);

    explicit SearchTreeNode();

    virtual ~SearchTreeNode();

    virtual void getRooms(RoomOutStream &stream, ParseEvent &event);

    virtual RoomCollection *insertRoom(ParseEvent &event);

    virtual void setChild(char, SearchTreeNode *);

    virtual void skipDown(RoomOutStream &stream, ParseEvent &event);
};


#endif
