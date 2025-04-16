#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "CGroupChar.h"
#include "mmapper2character.h"

class Mmapper2Group;

class NODISCARD GroupManagerApi final
{
private:
    Mmapper2Group &m_group;

public:
    explicit GroupManagerApi(Mmapper2Group &group)
        : m_group(group)
    {}
    ~GroupManagerApi() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(GroupManagerApi);

public:
    void refresh();
    NODISCARD SharedGroupChar getMember(const GroupId id);
    NODISCARD SharedGroupChar getMember(const CharacterName &name);
    NODISCARD const GroupVector &getMembers();
};
