// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GroupManagerApi.h"

#include "mmapper2character.h"
#include "mmapper2group.h"

void GroupManagerApi::refresh()
{
    return m_group.characterChanged(true);
}

SharedGroupChar GroupManagerApi::getMember(const GroupId id)
{
    if (id == INVALID_GROUPID) {
        throw std::invalid_argument("invalid id");
    }
    return m_group.getCharById(id);
}

SharedGroupChar GroupManagerApi::getMember(const CharacterName &name)
{
    if (name.isEmpty()) {
        throw std::invalid_argument("empty name");
    }
    return m_group.getCharByName(name);
}

const GroupVector &GroupManagerApi::getMembers()
{
    return m_group.selectAll();
}
