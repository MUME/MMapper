// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "groupselection.h"

#include "CGroupChar.h"

#include <cassert>
#include <vector>

GroupAdmin::~GroupAdmin() = default;
GroupRecipient::~GroupRecipient() = default;

GroupSelection::GroupSelection(GroupAdmin *const admin)
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
void GroupSelection::virt_receiveCharacters(GroupAdmin *const admin, GroupVector chars)
{
    assert(admin == m_admin);
    m_chars = std::move(chars);
}
