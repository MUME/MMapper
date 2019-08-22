#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../expandoracommon/parseevent.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "AbstractRoomVisitor.h"

class AbstractRoomVisitor;
class ParseEvent;

/*
 * ParseTree is effectively a 256-tree
 * that maps ParseEvent key to a shared RoomCollection value.
 */
class ParseTree final
{
public:
    struct Pimpl;

private:
    std::unique_ptr<Pimpl> m_pimpl;

public:
    explicit ParseTree(const bool useVerboseKeys = false);
    ~ParseTree();
    DELETE_CTORS_AND_ASSIGN_OPS(ParseTree);

public:
    /* REVISIT: Is it safe to convert these to const reference,
     * so we know that caller's copy of the event is unaffected? */
    SharedRoomCollection insertRoom(ParseEvent &event);
    void getRooms(AbstractRoomVisitor &stream, ParseEvent &event);
};
