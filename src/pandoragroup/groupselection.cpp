// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupselection.h"

#include <cassert>
#include <vector>

#include "CGroupChar.h"

GroupAdmin::~GroupAdmin() = default;
GroupRecipient::~GroupRecipient() = default;

GroupSelection::GroupSelection(GroupAdmin *admin)
    : m_admin(admin)
{}

GroupSelection::~GroupSelection()
{
    m_admin->releaseCharacters(this);
}

/**
 * @brief CGroupSelection::receiveCharacters
 * @param admin lock administrator
 * @param chars characters to insert
 */
void GroupSelection::virt_receiveCharacters(GroupAdmin *const admin, GroupVector in_chars)
{
    assert(admin == m_admin);
    chars = std::move(in_chars);
}
