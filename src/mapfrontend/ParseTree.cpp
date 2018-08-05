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

#include "ParseTree.h"

#include <memory>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/property.h"
#include "../global/utils.h"
#include "intermediatenode.h"
#include "roomcollection.h"

struct ParseTree::Pimpl
{
    explicit Pimpl() = default;
    virtual ~Pimpl() = default;
    virtual SharedRoomCollection insertRoom(ParseEvent &event) = 0;
    virtual void getRooms(AbstractRoomVisitor &stream, ParseEvent &event) = 0;
};

class TinyListParseTree final : public ParseTree::Pimpl
{
private:
    IntermediateNode root{};

public:
    explicit TinyListParseTree() = default;
    SharedRoomCollection insertRoom(ParseEvent &event) override { return root.insertRoom(event); }
    void getRooms(AbstractRoomVisitor &stream, ParseEvent &event) override
    {
        root.getRooms(stream, event);
    }
};

ParseTree::ParseTree()
    : m_pimpl{std::unique_ptr<Pimpl>(static_cast<Pimpl *>(new TinyListParseTree()))}
{}
ParseTree::~ParseTree() = default;

SharedRoomCollection ParseTree::insertRoom(ParseEvent &event)
{
    return m_pimpl->insertRoom(event);
}

void ParseTree::getRooms(AbstractRoomVisitor &stream, ParseEvent &event)
{
    m_pimpl->getRooms(stream, event);
}
