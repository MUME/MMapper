#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../expandoracommon/parseevent.h"
#include "../global/RuleOf5.h"
#include "../global/roomid.h"
#include "AbstractRoomVisitor.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class AbstractRoomVisitor;
class ParseEvent;

/// ParseTree is an 8-way hashmap combining key data from
/// ParseEvent's name, description, and terrain.
class ParseTree final
{
public:
    class ParseHashMap;

private:
    std::unique_ptr<ParseHashMap> m_pimpl;

public:
    ParseTree();
    ~ParseTree();
    DELETE_CTORS_AND_ASSIGN_OPS(ParseTree);

public:
    NODISCARD SharedRoomCollection insertRoom(const ParseEvent &event);
    void getRooms(AbstractRoomVisitor &stream, const ParseEvent &event);
};
