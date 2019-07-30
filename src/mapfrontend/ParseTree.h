#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    class ParseHashMap;

private:
    std::unique_ptr<ParseHashMap> m_pimpl;

public:
    explicit ParseTree(bool useVerboseKeys = false);
    ~ParseTree();
    DELETE_CTORS_AND_ASSIGN_OPS(ParseTree);

public:
    /* REVISIT: Is it safe to convert these to const reference,
     * so we know that caller's copy of the event is unaffected? */
    SharedRoomCollection insertRoom(ParseEvent &event);
    void getRooms(AbstractRoomVisitor &stream, ParseEvent &event);
};
