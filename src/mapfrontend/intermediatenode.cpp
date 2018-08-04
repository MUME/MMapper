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

#include "intermediatenode.h"

#include <memory>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/property.h"
#include "../global/utils.h"
#include "ByteArray.h"
#include "roomcollection.h"
#include "searchtreenode.h"

IntermediateNode::IntermediateNode(ParseEvent &event)
{
    if (Property *const prop = event.next()) {
        if (!prop->isSkipped()) {
            /* NOTE: This does not skip the first value */
            myChars = ByteArray{prop->rest()};
        }
    }
    event.prev();
}

SharedRoomCollection IntermediateNode::insertRoom(ParseEvent &event)
{
    if (event.next() == nullptr) {
        if (rooms == nullptr) {
            rooms = std::make_shared<RoomCollection>();
        }
        return rooms;
    }
    if (event.current()->isSkipped()) {
        return nullptr;
    }

    return SearchTreeNode::insertRoom(event);
}

void IntermediateNode::getRooms(AbstractRoomVisitor &stream, ParseEvent &event)
{
    if (event.next() == nullptr) {
        // REVISIT: should this allow null collection?
        deref(rooms).forEach(stream);
    } else if (event.current()->isSkipped()) {
        SearchTreeNode::skipDown(stream, event);
    } else {
        SearchTreeNode::getRooms(stream, event);
    }
}

void IntermediateNode::skipDown(AbstractRoomVisitor &stream, ParseEvent &event)
{
    getRooms(stream, event);
}
