#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../expandoracommon/room.h"
#include "../global/RuleOf5.h"

class AbstractRoomVisitor
{
public:
    AbstractRoomVisitor();
    virtual ~AbstractRoomVisitor();

public:
    virtual void visit(const Room *) = 0;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(AbstractRoomVisitor);
};
